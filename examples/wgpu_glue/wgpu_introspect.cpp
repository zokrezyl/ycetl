// SPDX-License-Identifier: MIT

// wgpu_introspect: parse a C header with libclang, build an in-memory
// type tree, emit two artefacts:
//
//   --python <path>   a Python module with ctypes.Structure + IntEnum
//                     definitions, ready to drive a WebGPU FFI binding.
//
//   --tree <path>     a C++ header that re-states the same tree as
//                     constexpr std::array initialisers (the "ycetl-shape"
//                     tree). A second TU can #include it and walk it at
//                     constexpr time.
//
// Runs at *build* time (libclang is a runtime library; constant evaluation
// can't call it). The constexpr round-trip happens in the consumer that
// reads the emitted header.

#include <clang-c/Index.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

struct cx_string_guard {
  CXString s;
  explicit cx_string_guard(CXString v) : s(v) {}
  ~cx_string_guard() { clang_disposeString(s); }
  [[nodiscard]] const char *c_str() const { return clang_getCString(s); }
  [[nodiscard]] std::string str() const {
    const char *p = clang_getCString(s);
    return p ? std::string(p) : std::string{};
  }
};

// ---- The tree ------------------------------------------------------------

struct field_info {
  std::string name;
  std::string type_spelling; // exactly what the header says
  std::int64_t offset_bits;  // -1 if unknown (e.g. flexible array)
  std::int64_t size_bytes;   // -1 if unknown
};

struct record_info {
  std::string name;         // e.g. "WGPUBindGroupEntry"
  std::int64_t size_bytes;  // -1 if forward-declared / opaque
  std::int64_t align_bytes; // ditto
  std::vector<field_info> fields;
  bool is_opaque() const { return size_bytes <= 0; }
};

struct enum_value {
  std::string name;
  long long value; // libclang gives a signed 64
};

struct enum_info {
  std::string name;
  std::string underlying; // canonical name of the underlying int type
  std::int64_t size_bytes;
  std::vector<enum_value> values;
};

struct typedef_info {
  std::string name;
  std::string underlying;
};

struct func_info {
  std::string name;
  std::string return_type;
  std::vector<std::string> arg_types;
  std::vector<std::string> arg_names;
};

struct api_tree {
  std::vector<record_info> records;
  std::vector<enum_info> enums;
  std::vector<typedef_info> typedefs;
  std::vector<func_info> functions;
};

// ---- libclang walker -----------------------------------------------------

// We only collect things actually declared by the main header, not by every
// transitively-included system header.
bool from_main_header(CXCursor c) {
  CXSourceLocation loc = clang_getCursorLocation(c);
  return clang_Location_isFromMainFile(loc) != 0;
}

std::string spelling(CXCursor c) {
  return cx_string_guard(clang_getCursorSpelling(c)).str();
}

std::string type_spelling(CXType t) {
  return cx_string_guard(clang_getTypeSpelling(t)).str();
}

void visit_struct(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  // Skip anonymous structs (no name → nothing to emit usefully).
  std::string name = spelling(decl);
  if (name.empty())
    return;
  // libclang emits a separate cursor for the forward declaration and
  // for the definition of `struct WGPUFoo`, plus another for the
  // typedef `typedef struct WGPUFoo WGPUFoo;`. Only walk the
  // definition; skip everything else so we don't get duplicate entries.
  if (!clang_equalCursors(decl, clang_getCursorDefinition(decl)))
    return;

  CXType t = clang_getCursorType(decl);

  record_info rec;
  rec.name = name;
  rec.size_bytes = clang_Type_getSizeOf(t);
  rec.align_bytes = clang_Type_getAlignOf(t);

  // CXType_getSizeOf returns negative CXTypeLayoutError codes for
  // incomplete / dependent types — clamp to -1 so downstream code can
  // treat "opaque" uniformly.
  if (rec.size_bytes < 0)
    rec.size_bytes = -1;
  if (rec.align_bytes < 0)
    rec.align_bytes = -1;

  clang_Type_visitFields(
      t,
      [](CXCursor field, CXClientData ud) {
        auto &rec = *static_cast<record_info *>(ud);
        field_info f;
        f.name = spelling(field);
        CXType ft = clang_getCursorType(field);
        f.type_spelling = type_spelling(ft);
        f.offset_bits = clang_Cursor_getOffsetOfField(field);
        f.size_bytes = clang_Type_getSizeOf(ft);
        if (f.offset_bits < 0)
          f.offset_bits = -1;
        if (f.size_bytes < 0)
          f.size_bytes = -1;
        rec.fields.push_back(std::move(f));
        return CXVisit_Continue;
      },
      &rec);

  tree.records.push_back(std::move(rec));
}

void visit_enum(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  std::string name = spelling(decl);
  if (name.empty())
    return;
  if (!clang_equalCursors(decl, clang_getCursorDefinition(decl)))
    return;

  enum_info e;
  e.name = name;
  CXType t = clang_getCursorType(decl);
  e.size_bytes = clang_Type_getSizeOf(t);
  if (e.size_bytes < 0)
    e.size_bytes = -1;

  CXType ut = clang_getEnumDeclIntegerType(decl);
  e.underlying = type_spelling(ut);

  struct ctx {
    enum_info *e;
  };
  ctx c{&e};
  clang_visitChildren(
      decl,
      [](CXCursor child, CXCursor /*parent*/, CXClientData ud) {
        if (clang_getCursorKind(child) == CXCursor_EnumConstantDecl) {
          auto &c = *static_cast<ctx *>(ud);
          enum_value ev;
          ev.name = spelling(child);
          ev.value = clang_getEnumConstantDeclValue(child);
          c.e->values.push_back(std::move(ev));
        }
        return CXChildVisit_Continue;
      },
      &c);

  tree.enums.push_back(std::move(e));
}

void visit_typedef(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  typedef_info td;
  td.name = spelling(decl);
  td.underlying = type_spelling(clang_getTypedefDeclUnderlyingType(decl));
  tree.typedefs.push_back(std::move(td));
}

void visit_function(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  func_info f;
  f.name = spelling(decl);
  CXType ft = clang_getCursorType(decl);
  f.return_type = type_spelling(clang_getResultType(ft));
  int nargs = clang_Cursor_getNumArguments(decl);
  for (int i = 0; i < nargs; ++i) {
    CXCursor a = clang_Cursor_getArgument(decl, i);
    f.arg_types.push_back(type_spelling(clang_getCursorType(a)));
    f.arg_names.push_back(spelling(a));
  }
  tree.functions.push_back(std::move(f));
}

CXChildVisitResult tu_visitor(CXCursor c, CXCursor /*parent*/,
                              CXClientData ud) {
  auto &tree = *static_cast<api_tree *>(ud);
  switch (clang_getCursorKind(c)) {
  case CXCursor_StructDecl:
    visit_struct(c, tree);
    break;
  case CXCursor_EnumDecl:
    visit_enum(c, tree);
    break;
  case CXCursor_TypedefDecl:
    visit_typedef(c, tree);
    break;
  case CXCursor_FunctionDecl:
    visit_function(c, tree);
    break;
  default:
    break;
  }
  return CXChildVisit_Continue;
}

// ---- Python emitter ------------------------------------------------------

// Map a libclang type spelling (or a typedef name we've seen) to a Python
// ctypes expression. Anything we can't classify becomes c_void_p with a
// `# unmapped: <spelling>` comment — the Python side will still load.
struct py_type_map {
  std::unordered_map<std::string, std::string> typedef_to_ctypes;
  std::unordered_set<std::string> enum_names;
  std::unordered_set<std::string> record_names;

  std::string lookup(std::string s) const {
    // Strip leading/trailing whitespace.
    while (!s.empty() && s.back() == ' ')
      s.pop_back();
    while (!s.empty() && s.front() == ' ')
      s.erase(s.begin(), s.begin() + 1);

    // const T → T
    if (s.rfind("const ", 0) == 0)
      s = s.substr(6);

    // Pointer → c_void_p (we don't reconstruct POINTER(T) here — for
    // a WebGPU ctypes binding that's almost always what you want for
    // opaque handles and out-params; structures are passed by-value
    // or as pointers handled at call sites).
    if (!s.empty() && s.back() == '*')
      return "c_void_p";

    // Fixed-size arrays:   T[N]   →   T * N
    if (auto lb = s.find('['); lb != std::string::npos) {
      auto rb = s.find(']', lb);
      if (rb != std::string::npos) {
        std::string elem = s.substr(0, lb);
        std::string n = s.substr(lb + 1, rb - lb - 1);
        while (!elem.empty() && elem.back() == ' ')
          elem.pop_back();
        return lookup(elem) + " * " + n;
      }
    }

    static const std::unordered_map<std::string, std::string> base = {
        {"void", "None"},
        {"_Bool", "c_bool"},
        {"bool", "c_bool"},
        {"char", "c_char"},
        {"signed char", "c_byte"},
        {"unsigned char", "c_ubyte"},
        {"short", "c_short"},
        {"unsigned short", "c_ushort"},
        {"int", "c_int"},
        {"unsigned int", "c_uint"},
        {"long", "c_long"},
        {"unsigned long", "c_ulong"},
        {"long long", "c_longlong"},
        {"unsigned long long", "c_ulonglong"},
        {"float", "c_float"},
        {"double", "c_double"},
        {"size_t", "c_size_t"},
        {"int8_t", "c_int8"},
        {"int16_t", "c_int16"},
        {"int32_t", "c_int32"},
        {"int64_t", "c_int64"},
        {"uint8_t", "c_uint8"},
        {"uint16_t", "c_uint16"},
        {"uint32_t", "c_uint32"},
        {"uint64_t", "c_uint64"},
    };
    if (auto it = base.find(s); it != base.end())
      return it->second;

    if (auto it = typedef_to_ctypes.find(s); it != typedef_to_ctypes.end())
      return it->second;

    if (enum_names.count(s))
      return s; // IntEnum classes are c-int-shaped
    if (record_names.count(s))
      return s;        // Structure subclass
    return "c_void_p"; // safe-ish default
  }
};

py_type_map build_py_map(const api_tree &tree) {
  py_type_map m;
  for (const auto &e : tree.enums)
    m.enum_names.insert(e.name);
  for (const auto &r : tree.records)
    m.record_names.insert(r.name);
  // Two-pass typedef resolution so "WGPUFlags = uint64_t" lands as
  // c_uint64, and a later "typedef WGPUFlags WGPUBufferUsage" picks
  // c_uint64 too.
  bool progress = true;
  while (progress) {
    progress = false;
    for (const auto &t : tree.typedefs) {
      if (m.typedef_to_ctypes.count(t.name))
        continue;
      std::string r = m.lookup(t.underlying);
      // The lookup returns the typedef *name* itself for unknowns,
      // since the typedef hasn't been registered yet. Skip those.
      if (r == "c_void_p" && t.underlying != "void *"
          && t.underlying.find('*') == std::string::npos
          && t.underlying.find('[') == std::string::npos)
        continue;
      m.typedef_to_ctypes[t.name] = r;
      progress = true;
    }
  }
  // Anything still unresolved gets c_void_p so the module is loadable.
  for (const auto &t : tree.typedefs)
    m.typedef_to_ctypes.try_emplace(t.name, "c_void_p");
  return m;
}

void emit_python(const api_tree &tree, const std::string &path) {
  std::ofstream out(path);
  out << "# Generated by wgpu_introspect. Do not edit by hand.\n"
         "# Source: dawn/webgpu.h\n"
         "from ctypes import (\n"
         "    Structure, CFUNCTYPE, POINTER,\n"
         "    c_bool, c_char, c_byte, c_ubyte,\n"
         "    c_short, c_ushort, c_int, c_uint,\n"
         "    c_long, c_ulong, c_longlong, c_ulonglong,\n"
         "    c_int8, c_int16, c_int32, c_int64,\n"
         "    c_uint8, c_uint16, c_uint32, c_uint64,\n"
         "    c_float, c_double, c_size_t, c_void_p,\n"
         ")\n"
         "from enum import IntEnum\n\n";

  // Enums.
  for (const auto &e : tree.enums) {
    out << "class " << e.name << "(IntEnum):\n";
    if (e.values.empty()) {
      out << "    pass\n\n";
      continue;
    }
    // Deduplicate by value: ctypes' IntEnum forbids duplicate values
    // unless we mark them as aliases — webgpu does have aliases
    // (e.g. _Force32). We strip the common enum prefix when one
    // exists, and let later identical-value names become aliases by
    // skipping them entirely. (Python's IntEnum auto-aliases names
    // sharing a value, but we keep the file readable.)
    std::unordered_set<long long> seen;
    for (const auto &v : e.values) {
      if (!seen.insert(v.value).second) {
        out << "    # " << v.name << " = " << v.value << "  (alias)\n";
        continue;
      }
      out << "    " << v.name << " = " << v.value << "\n";
    }
    out << "\n";
  }

  // Forward-declare struct classes so cross-references in _fields_ work
  // (pointer-typed fields become c_void_p and don't need the real layout).
  for (const auto &r : tree.records)
    out << "class " << r.name << "(Structure): pass\n";
  out << "\n";

  // Topologically sort records by by-value dependencies. ctypes
  // finalises a Structure the moment anything queries its layout
  // (including another Structure referencing it by value in _fields_),
  // and an assignment to _fields_ on a finalised class fails with
  //   AttributeError: _fields_ is final
  // Pointer-typed fields are c_void_p in the emit and so contribute no
  // dependency — only direct struct-by-value fields do.
  auto record_idx = [&] {
    std::unordered_map<std::string, std::size_t> m;
    for (std::size_t i = 0; i < tree.records.size(); ++i)
      m[tree.records[i].name] = i;
    return m;
  }();

  std::vector<std::vector<std::size_t>> deps(tree.records.size());
  for (std::size_t i = 0; i < tree.records.size(); ++i) {
    for (const auto &f : tree.records[i].fields) {
      // Pointer / array-of-pointer fields don't need the target laid out.
      if (f.type_spelling.find('*') != std::string::npos)
        continue;
      // Find a bare-name dependency: strip const + leading qualifiers.
      std::string s = f.type_spelling;
      if (s.rfind("const ", 0) == 0)
        s = s.substr(6);
      if (auto sp = s.rfind(' '); sp != std::string::npos)
        s = s.substr(sp + 1);
      if (auto br = s.find('['); br != std::string::npos)
        s = s.substr(0, br);
      if (auto it = record_idx.find(s); it != record_idx.end())
        deps[i].push_back(it->second);
    }
  }

  std::vector<std::size_t> order;
  order.reserve(tree.records.size());
  std::vector<int> mark(tree.records.size(),
                        0); // 0=unseen 1=in-progress 2=done
  auto visit = [&](auto &self, std::size_t i) -> void {
    if (mark[i] == 2)
      return;
    if (mark[i] == 1)
      return; // cycle — break it (C structs can't truly cycle by-value)
    mark[i] = 1;
    for (auto d : deps[i])
      self(self, d);
    mark[i] = 2;
    order.push_back(i);
  };
  for (std::size_t i = 0; i < tree.records.size(); ++i)
    visit(visit, i);

  // Typedefs: aliasing only. Crucially, C declares
  //   typedef enum WGPUTextureFormat WGPUTextureFormat;
  //   typedef struct WGPUColor { ... } WGPUColor;
  // so the typedef name often collides with the enum/struct name we
  // already declared as an IntEnum / Structure class. Emitting
  // `WGPUTextureFormat = c_void_p` would clobber the class.
  py_type_map pm = build_py_map(tree);
  for (const auto &t : tree.typedefs) {
    if (pm.enum_names.count(t.name) || pm.record_names.count(t.name))
      continue;
    auto it = pm.typedef_to_ctypes.find(t.name);
    if (it == pm.typedef_to_ctypes.end())
      continue;
    if (it->second == t.name)
      continue;
    out << t.name << " = " << it->second << "\n";
  }
  out << "\n";

  // Struct field tables — emitted in topologically-sorted order.
  for (std::size_t idx : order) {
    const auto &r = tree.records[idx];
    if (r.is_opaque()) {
      out << "# " << r.name << ": opaque (no layout exposed)\n";
      continue;
    }
    out << r.name << "._fields_ = [\n";
    for (const auto &f : r.fields) {
      std::string ct = pm.lookup(f.type_spelling);
      out << "    (\"" << f.name << "\", " << ct
          << "),  # offset=" << (f.offset_bits / 8) << " size=" << f.size_bytes
          << " src=" << f.type_spelling << "\n";
    }
    out << "]\n\n";
  }

  // Functions as CFUNCTYPE prototypes (ready to bind via a dlopen'd
  // libwebgpu_dawn.so on the Python side).
  out << "# Function prototypes (bind with cdll.LoadLibrary(...).<name>)\n";
  for (const auto &f : tree.functions) {
    out << f.name << "_proto = CFUNCTYPE(";
    out << pm.lookup(f.return_type);
    for (const auto &a : f.arg_types)
      out << ", " << pm.lookup(a);
    out << ")\n";
  }
}

// ---- C++ constexpr-tree emitter -----------------------------------------

// Escape a string for a C++ string literal.
std::string esc(const std::string &s) {
  std::string r;
  r.reserve(s.size() + 2);
  for (char c : s) {
    if (c == '"' || c == '\\')
      r.push_back('\\');
    r.push_back(c);
  }
  return r;
}

void emit_tree(const api_tree &tree, const std::string &path) {
  std::ofstream out(path);
  out << "// Generated by wgpu_introspect. Do not edit by hand.\n"
         "// Source: dawn/webgpu.h\n"
         "//\n"
         "// The whole API description is a single constexpr value tree of\n"
         "// std::array + std::string_view + plain structs. Anything\n"
         "// consteval / constexpr can walk it at compile time.\n"
         "#pragma once\n"
         "#include <array>\n"
         "#include <cstdint>\n"
         "#include <string_view>\n\n"
         "namespace wgpu_tree {\n\n"
         "struct field_info {\n"
         "    std::string_view name;\n"
         "    std::string_view type_spelling;\n"
         "    std::int64_t offset_bits;\n"
         "    std::int64_t size_bytes;\n"
         "};\n\n"
         "template <std::size_t N>\n"
         "struct record_info {\n"
         "    std::string_view name;\n"
         "    std::int64_t size_bytes;\n"
         "    std::int64_t align_bytes;\n"
         "    std::array<field_info, N> fields;\n"
         "};\n\n"
         "struct enum_value {\n"
         "    std::string_view name;\n"
         "    long long value;\n"
         "};\n\n"
         "template <std::size_t N>\n"
         "struct enum_info {\n"
         "    std::string_view name;\n"
         "    std::string_view underlying;\n"
         "    std::int64_t size_bytes;\n"
         "    std::array<enum_value, N> values;\n"
         "};\n\n";

  // Counts (so a consumer can static_assert against them).
  out << "inline constexpr std::size_t record_count   = " << tree.records.size()
      << ";\n";
  out << "inline constexpr std::size_t enum_count     = " << tree.enums.size()
      << ";\n";
  out << "inline constexpr std::size_t typedef_count  = "
      << tree.typedefs.size() << ";\n";
  out << "inline constexpr std::size_t function_count = "
      << tree.functions.size() << ";\n\n";

  // Records: one constexpr variable per record (sized template means
  // we can't put them all in one homogeneous array, but we expose them
  // through a name table below for runtime lookup).
  for (std::size_t i = 0; i < tree.records.size(); ++i) {
    const auto &r = tree.records[i];
    out << "inline constexpr record_info<" << r.fields.size() << "> rec_" << i
        << " = {\n"
        << "    \"" << esc(r.name) << "\",\n"
        << "    " << r.size_bytes << ", " << r.align_bytes << ",\n"
        << "    {{\n";
    for (const auto &f : r.fields) {
      out << "        {\"" << esc(f.name) << "\", \"" << esc(f.type_spelling)
          << "\", " << f.offset_bits << ", " << f.size_bytes << "},\n";
    }
    out << "    }}\n};\n";
  }

  // Same per-enum.
  for (std::size_t i = 0; i < tree.enums.size(); ++i) {
    const auto &e = tree.enums[i];
    out << "inline constexpr enum_info<" << e.values.size() << "> enm_" << i
        << " = {\n"
        << "    \"" << esc(e.name) << "\",\n"
        << "    \"" << esc(e.underlying) << "\",\n"
        << "    " << e.size_bytes << ",\n"
        << "    {{\n";
    for (const auto &v : e.values) {
      out << "        {\"" << esc(v.name) << "\", " << v.value << "LL},\n";
    }
    out << "    }}\n};\n";
  }

  // Name index — homogeneous, walkable at constexpr time, lets a
  // consumer find a record/enum by name without instantiating every
  // record_info<N> individually.
  out << "\nstruct named_record { std::string_view name; const void *ptr; };\n"
      << "inline constexpr std::array<named_record, record_count> records = "
         "{{\n";
  for (std::size_t i = 0; i < tree.records.size(); ++i)
    out << "    {\"" << esc(tree.records[i].name) << "\", &rec_" << i << "},\n";
  out << "}};\n\n";

  out << "struct named_enum { std::string_view name; const void *ptr; };\n"
      << "inline constexpr std::array<named_enum, enum_count> enums = {{\n";
  for (std::size_t i = 0; i < tree.enums.size(); ++i)
    out << "    {\"" << esc(tree.enums[i].name) << "\", &enm_" << i << "},\n";
  out << "}};\n\n";

  out << "} // namespace wgpu_tree\n";
}

// ---- CLI -----------------------------------------------------------------

struct cli_args {
  std::string header;
  std::string python_out;
  std::string tree_out;
};

bool parse_cli(int argc, char **argv, cli_args &out) {
  for (int i = 1; i < argc; ++i) {
    std::string_view a = argv[i];
    auto next = [&]() -> const char * {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %s\n", argv[i]);
        return nullptr;
      }
      return argv[++i];
    };
    if (a == "--header") {
      auto v = next();
      if (!v)
        return false;
      out.header = v;
    } else if (a == "--python") {
      auto v = next();
      if (!v)
        return false;
      out.python_out = v;
    } else if (a == "--tree") {
      auto v = next();
      if (!v)
        return false;
      out.tree_out = v;
    } else {
      std::fprintf(stderr, "unknown arg: %s\n", argv[i]);
      return false;
    }
  }
  if (out.header.empty()) {
    std::fprintf(stderr, "usage: wgpu_introspect --header H.h [--python "
                         "out.py] [--tree out.hpp]\n");
    return false;
  }
  return true;
}

} // namespace

int main(int argc, char **argv) {
  cli_args args;
  if (!parse_cli(argc, argv, args))
    return 1;

  // Parse the header. -xc so we don't trip on C++ keywords; we want C.
  // Resource dir comes from CMake — see CMakeLists.txt — so the
  // built-in headers (stddef.h, stdint.h, …) the parsed header pulls in
  // are found.
  std::vector<const char *> clang_args = {"-xc", "-std=c11"};
#ifdef WGPU_CLANG_RESOURCE_DIR
  if (std::string_view(WGPU_CLANG_RESOURCE_DIR).size() > 0) {
    clang_args.push_back("-resource-dir");
    clang_args.push_back(WGPU_CLANG_RESOURCE_DIR);
  }
#endif
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit tu = nullptr;
  auto err = clang_parseTranslationUnit2(
      index, args.header.c_str(), clang_args.data(), int(clang_args.size()),
      nullptr, 0,
      CXTranslationUnit_DetailedPreprocessingRecord
          | CXTranslationUnit_SkipFunctionBodies,
      &tu);
  if (err != CXError_Success || tu == nullptr) {
    std::fprintf(stderr, "failed to parse %s (libclang err=%d)\n",
                 args.header.c_str(), int(err));
    clang_disposeIndex(index);
    return 1;
  }

  // Diagnostics are interesting but non-fatal — the dawn header pulls in
  // standard headers and we don't pass -I; we'll still get the WGPU
  // declarations because they live in the main file.
  unsigned ndiag = clang_getNumDiagnostics(tu);
  unsigned shown = 0;
  for (unsigned i = 0; i < ndiag && shown < 5; ++i) {
    CXDiagnostic d = clang_getDiagnostic(tu, i);
    if (clang_getDiagnosticSeverity(d) >= CXDiagnostic_Error) {
      cx_string_guard s(
          clang_formatDiagnostic(d, clang_defaultDiagnosticDisplayOptions()));
      std::fprintf(stderr, "%s\n", s.c_str());
      ++shown;
    }
    clang_disposeDiagnostic(d);
  }

  api_tree tree;
  clang_visitChildren(clang_getTranslationUnitCursor(tu), tu_visitor, &tree);

  // Deterministic order — makes the emitted files diff-friendly.
  std::sort(tree.records.begin(), tree.records.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  std::sort(tree.enums.begin(), tree.enums.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  std::sort(tree.typedefs.begin(), tree.typedefs.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  std::sort(tree.functions.begin(), tree.functions.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });

  std::cerr << "wgpu_introspect: records=" << tree.records.size()
            << " enums=" << tree.enums.size()
            << " typedefs=" << tree.typedefs.size()
            << " funcs=" << tree.functions.size() << '\n';

  if (!args.python_out.empty())
    emit_python(tree, args.python_out);
  if (!args.tree_out.empty())
    emit_tree(tree, args.tree_out);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);
  return 0;
}
