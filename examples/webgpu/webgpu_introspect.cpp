// SPDX-License-Identifier: MIT

// webgpu_introspect: parse a C header with libclang, build an in-memory
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
#include <cctype>
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
  std::string ctypes_type;   // ABI-correct Python ctypes expression
  std::string record_dep;    // by-value struct tag this field embeds, if any
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
  // Function-pointer typedefs (the WGPU*Callback family) carry a ctypes
  // CFUNCTYPE signature so the emitter can produce a usable callback type.
  bool is_callback = false;
  std::string callback_ret;
  std::vector<std::string> callback_args;
};

// A file-scope integer constant: either a `static const` flag (the
// WGPU*Usage_* / WGPUColorWriteMask_* / ... families) or an object-like
// integer macro (WGPU_DEPTH_SLICE_UNDEFINED, ...). `value` is a ready-to-emit
// Python integer literal.
struct constant_info {
  std::string name;
  std::string value;
};

struct func_info {
  std::string name;
  std::string return_type;
  std::string return_ctype;              // ABI-correct ctypes expression
  std::vector<std::string> arg_types;
  std::vector<std::string> arg_ctypes;   // ABI-correct ctypes expressions
  std::vector<std::string> arg_names;
};

struct api_tree {
  std::vector<record_info> records;
  std::vector<enum_info> enums;
  std::vector<typedef_info> typedefs;
  std::vector<func_info> functions;
  std::vector<constant_info> constants;
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

// Map a libclang type to a Python ctypes expression that preserves the real
// C layout. The string-based mapper this replaces flattened every embedded
// by-value struct and every enum to c_void_p (8 bytes), which corrupted the
// size/offsets of any descriptor carrying a WGPUStringView label or a
// WGPUChainedStruct chain. Here we dispatch on the actual CXType kind:
//   - embedded struct by value -> its nested Structure class (by tag name)
//   - enum                     -> its real underlying integer (4 bytes)
//   - fixed array              -> "elem * N"
//   - pointer                  -> c_void_p
//   - typedef / elaborated     -> resolve to the canonical type
std::string ctypes_for(CXType t) {
  switch (t.kind) {
  case CXType_Void:
    return "None";
  case CXType_Bool:
    return "c_bool";
  case CXType_Char_S:
  case CXType_SChar:
    return "c_char";
  case CXType_Char_U:
  case CXType_UChar:
    return "c_ubyte";
  case CXType_Short:
    return "c_short";
  case CXType_UShort:
    return "c_ushort";
  case CXType_Int:
    return "c_int";
  case CXType_UInt:
    return "c_uint";
  case CXType_Long:
    return "c_long";
  case CXType_ULong:
    return "c_ulong";
  case CXType_LongLong:
    return "c_longlong";
  case CXType_ULongLong:
    return "c_ulonglong";
  case CXType_Float:
    return "c_float";
  case CXType_Double:
    return "c_double";
  case CXType_Pointer:
    return "c_void_p";
  case CXType_Enum:
    return ctypes_for(clang_getEnumDeclIntegerType(clang_getTypeDeclaration(t)));
  case CXType_Record: {
    std::string name = spelling(clang_getTypeDeclaration(t));
    if (name.empty()) {
      // Anonymous record embedded by value — keep the size as a byte blob
      // so the surrounding layout stays correct (field access is lost).
      long long size_bytes = clang_Type_getSizeOf(t);
      return "(c_char * " + std::to_string(size_bytes > 0 ? size_bytes : 0)
             + ")";
    }
    return name;
  }
  case CXType_ConstantArray:
    return ctypes_for(clang_getArrayElementType(t)) + " * "
           + std::to_string(clang_getArraySize(t));
  case CXType_Typedef:
  case CXType_Elaborated:
    return ctypes_for(clang_getCanonicalType(t));
  default: {
    // Unknown kind: keep the right footprint so the struct size survives.
    long long size_bytes = clang_Type_getSizeOf(t);
    switch (size_bytes) {
    case 1:
      return "c_uint8";
    case 2:
      return "c_uint16";
    case 4:
      return "c_uint32";
    case 8:
      return "c_uint64";
    default:
      return "c_void_p";
    }
  }
  }
}

// If `t` is (an array of) a by-value struct, return that struct's tag name so
// the emitter can order its definition before any struct that embeds it.
// Pointers, enums and scalars have no by-value layout dependency.
std::string record_dependency(CXType t) {
  CXType canonical = clang_getCanonicalType(t);
  while (canonical.kind == CXType_ConstantArray
         || canonical.kind == CXType_IncompleteArray)
    canonical = clang_getCanonicalType(clang_getArrayElementType(canonical));
  if (canonical.kind == CXType_Record)
    return spelling(clang_getTypeDeclaration(canonical));
  return "";
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
        f.ctypes_type = ctypes_for(ft);
        f.record_dep = record_dependency(ft);
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
  CXType underlying = clang_getTypedefDeclUnderlyingType(decl);
  td.underlying = type_spelling(underlying);

  // A function-pointer typedef (e.g. WGPURequestAdapterCallback) — capture its
  // signature so the emitter can produce a CFUNCTYPE the caller can wrap.
  CXType canonical = clang_getCanonicalType(underlying);
  if (canonical.kind == CXType_Pointer) {
    CXType pointee = clang_getPointeeType(canonical);
    if (pointee.kind == CXType_FunctionProto
        || pointee.kind == CXType_FunctionNoProto) {
      td.is_callback = true;
      td.callback_ret = ctypes_for(clang_getResultType(pointee));
      int nargs = clang_getNumArgTypes(pointee);
      for (int i = 0; i < nargs; ++i)
        td.callback_args.push_back(ctypes_for(clang_getArgType(pointee, i)));
    }
  }
  tree.typedefs.push_back(std::move(td));
}

// A file-scope `static const` integer (the WGPU*Usage_* flag families and
// friends) — captured by evaluating its initializer.
void visit_constant(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  CXType t = clang_getCursorType(decl);
  if (!clang_isConstQualifiedType(t))
    return;
  CXEvalResult ev = clang_Cursor_Evaluate(decl);
  if (!ev)
    return;
  if (clang_EvalResult_getKind(ev) == CXEval_Int) {
    constant_info ci;
    ci.name = spelling(decl);
    ci.value = std::to_string(clang_EvalResult_getAsLongLong(ev));
    tree.constants.push_back(std::move(ci));
  }
  clang_EvalResult_dispose(ev);
}

// An object-like integer macro (WGPU_DEPTH_SLICE_UNDEFINED = (UINT32_MAX), …).
// Only single-value bodies are resolved; anything else is skipped.
void visit_macro(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl) || clang_Cursor_isMacroFunctionLike(decl))
    return;
  std::string name = spelling(decl);
  if (name.rfind("WGPU", 0) != 0)
    return; // skip internal helper macros (WGPU_NULLABLE, _wgpu_*, …)

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(decl);
  CXToken *tokens = nullptr;
  unsigned count = 0;
  clang_tokenize(tu, clang_getCursorExtent(decl), &tokens, &count);
  std::vector<std::string> body; // tokens after the macro name, sans parens
  for (unsigned i = 1; i < count; ++i) {
    std::string tok = cx_string_guard(clang_getTokenSpelling(tu, tokens[i])).str();
    if (tok != "(" && tok != ")")
      body.push_back(tok);
  }
  clang_disposeTokens(tu, tokens, count);
  if (body.size() != 1)
    return;

  static const std::unordered_map<std::string, std::string> limits = {
      {"UINT8_MAX", "0xff"},          {"UINT16_MAX", "0xffff"},
      {"UINT32_MAX", "0xffffffff"},   {"UINT64_MAX", "0xffffffffffffffff"},
      {"SIZE_MAX", "0xffffffffffffffff"},
      {"INT32_MAX", "0x7fffffff"},    {"INT64_MAX", "0x7fffffffffffffff"},
  };
  std::string tok = body[0];
  std::string value;
  if (auto it = limits.find(tok); it != limits.end()) {
    value = it->second;
  } else {
    // Strip integer-literal suffixes (u/U/l/L) and accept decimal / 0x forms.
    std::string digits = tok;
    while (!digits.empty()
           && (digits.back() == 'u' || digits.back() == 'U' || digits.back() == 'l'
               || digits.back() == 'L'))
      digits.pop_back();
    bool hex = digits.rfind("0x", 0) == 0 || digits.rfind("0X", 0) == 0;
    std::size_t start = hex ? 2 : 0;
    if (digits.size() > start
        && std::all_of(digits.begin() + start, digits.end(), [hex](char c) {
             return hex ? std::isxdigit((unsigned char)c)
                        : std::isdigit((unsigned char)c);
           }))
      value = digits;
  }
  if (!value.empty())
    tree.constants.push_back({name, value});
}

void visit_function(CXCursor decl, api_tree &tree) {
  if (!from_main_header(decl))
    return;
  func_info f;
  f.name = spelling(decl);
  CXType ft = clang_getCursorType(decl);
  CXType rt = clang_getResultType(ft);
  f.return_type = type_spelling(rt);
  f.return_ctype = ctypes_for(rt);
  int nargs = clang_Cursor_getNumArguments(decl);
  for (int i = 0; i < nargs; ++i) {
    CXCursor a = clang_Cursor_getArgument(decl, i);
    CXType at = clang_getCursorType(a);
    f.arg_types.push_back(type_spelling(at));
    f.arg_ctypes.push_back(ctypes_for(at));
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
  case CXCursor_VarDecl:
    visit_constant(c, tree);
    break;
  case CXCursor_MacroDefinition:
    visit_macro(c, tree);
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
  out << "# Generated by webgpu_introspect. Do not edit by hand.\n"
         "# Source: webgpu.h\n"
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

  // Constants: the `static const` flag families (WGPUBufferUsage_*, …) and
  // object-like integer macros (WGPU_DEPTH_SLICE_UNDEFINED, …). These are part
  // of the API surface but are neither enums nor structs, so callers would
  // otherwise have to hand-copy the magic numbers.
  if (!tree.constants.empty()) {
    out << "# Flag and sentinel constants.\n";
    for (const auto &c : tree.constants)
      out << c.name << " = " << c.value << "\n";
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
      // Only by-value embedded structs create a layout dependency; the
      // parser already resolved which struct that is (empty for pointers,
      // enums and scalars).
      if (f.record_dep.empty())
        continue;
      if (auto it = record_idx.find(f.record_dep); it != record_idx.end())
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
    if (t.is_callback)
      continue; // emitted as CFUNCTYPE after the struct layouts are known
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
      out << "    (\"" << f.name << "\", " << f.ctypes_type
          << "),  # offset=" << (f.offset_bits / 8) << " size=" << f.size_bytes
          << " src=" << f.type_spelling << "\n";
    }
    out << "]\n\n";
  }

  // Callback function-pointer types (WGPU*Callback). Emitted here, after the
  // struct layouts, because their signatures embed structs by value (e.g.
  // WGPUStringView) that must already be complete.
  out << "# Callback types — wrap a Python function: cb = WGPUFooCallback(fn)\n";
  for (const auto &t : tree.typedefs) {
    if (!t.is_callback)
      continue;
    out << t.name << " = CFUNCTYPE("
        << (t.callback_ret.empty() ? "None" : t.callback_ret);
    for (const auto &a : t.callback_args)
      out << ", " << a;
    out << ")\n";
  }
  out << "\n";

  // Function table + loader. `load(path)` binds every function on the dlopen'd
  // library with its real restype/argtypes, so callers never hand-write FFI
  // signatures — they just `lib = webgpu_ctypes.load("libwebgpu_dawn.so")`.
  out << "_FUNCTIONS = [\n";
  for (const auto &f : tree.functions) {
    out << "    (\"" << f.name << "\", "
        << (f.return_ctype.empty() ? "None" : f.return_ctype) << ", [";
    for (std::size_t i = 0; i < f.arg_ctypes.size(); ++i)
      out << (i ? ", " : "") << f.arg_ctypes[i];
    out << "]),\n";
  }
  out << "]\n\n";

  out << "def load(path):\n"
         "    \"\"\"dlopen the WebGPU library and bind every function with its\n"
         "    ctypes restype/argtypes. Returns the ctypes.CDLL.\"\"\"\n"
         "    import ctypes\n"
         "    lib = ctypes.CDLL(path)\n"
         "    for name, restype, argtypes in _FUNCTIONS:\n"
         "        try:\n"
         "            fn = getattr(lib, name)\n"
         "        except AttributeError:\n"
         "            continue  # symbol not exported by this build\n"
         "        fn.restype = restype\n"
         "        fn.argtypes = argtypes\n"
         "    return lib\n";
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

// A quoted Python-ctypes text literal stored verbatim in the tree (e.g. the
// string "WGPUStringView" or "c_void_p" the emitter prints raw into the .py).
std::string qstr(const std::string &s) { return "\"" + esc(s) + "\""; }

void emit_tree(const api_tree &tree, const std::string &path) {
  std::ofstream out(path);

  // Recompute the exact orderings emit_python uses, so a constexpr consumer
  // walking this tree can reproduce the Python bindings byte-for-byte.
  std::unordered_map<std::string, std::size_t> record_idx;
  for (std::size_t i = 0; i < tree.records.size(); ++i)
    record_idx[tree.records[i].name] = i;

  // Topological order by by-value struct dependencies (same DFS as emit_python).
  std::vector<std::vector<std::size_t>> deps(tree.records.size());
  for (std::size_t i = 0; i < tree.records.size(); ++i)
    for (const auto &f : tree.records[i].fields)
      if (!f.record_dep.empty())
        if (auto it = record_idx.find(f.record_dep); it != record_idx.end())
          deps[i].push_back(it->second);
  std::vector<std::size_t> topo;
  std::vector<int> mark(tree.records.size(), 0);
  auto visit = [&](auto &self, std::size_t i) -> void {
    if (mark[i])
      return;
    mark[i] = 1;
    for (auto d : deps[i])
      self(self, d);
    mark[i] = 2;
    topo.push_back(i);
  };
  for (std::size_t i = 0; i < tree.records.size(); ++i)
    visit(visit, i);

  // Filtered typedef aliases (same filter as emit_python).
  py_type_map pm = build_py_map(tree);
  std::vector<std::string> alias_lits;
  for (const auto &t : tree.typedefs) {
    if (pm.enum_names.count(t.name) || pm.record_names.count(t.name))
      continue;
    if (t.is_callback)
      continue;
    auto it = pm.typedef_to_ctypes.find(t.name);
    if (it == pm.typedef_to_ctypes.end() || it->second == t.name)
      continue;
    alias_lits.push_back("{" + qstr(t.name) + ", " + qstr(it->second) + "}");
  }

  // Flatten everything into homogeneous pools.
  std::vector<std::string> field_lits, record_lits;
  for (const auto &r : tree.records) {
    std::size_t begin = field_lits.size();
    for (const auto &f : r.fields)
      field_lits.push_back("{" + qstr(f.name) + ", " + qstr(f.ctypes_type) + ", "
                           + qstr(f.type_spelling) + ", "
                           + std::to_string(f.offset_bits / 8) + ", "
                           + std::to_string(f.size_bytes) + "}");
    record_lits.push_back("{" + qstr(r.name) + ", "
                          + (r.is_opaque() ? "true" : "false") + ", "
                          + std::to_string(begin) + ", "
                          + std::to_string(r.fields.size()) + "}");
  }

  std::vector<std::string> enumv_lits, enum_lits;
  for (const auto &e : tree.enums) {
    std::size_t begin = enumv_lits.size();
    std::unordered_set<long long> seen;
    for (const auto &v : e.values) {
      bool is_alias = !seen.insert(v.value).second;
      enumv_lits.push_back("{" + qstr(v.name) + ", " + std::to_string(v.value)
                           + "LL, " + (is_alias ? "true" : "false") + "}");
    }
    enum_lits.push_back("{" + qstr(e.name) + ", " + std::to_string(begin) + ", "
                        + std::to_string(e.values.size()) + "}");
  }

  std::vector<std::string> cbarg_lits, callback_lits;
  for (const auto &t : tree.typedefs) {
    if (!t.is_callback)
      continue;
    std::size_t begin = cbarg_lits.size();
    for (const auto &a : t.callback_args)
      cbarg_lits.push_back(qstr(a));
    callback_lits.push_back(
        "{" + qstr(t.name) + ", "
        + qstr(t.callback_ret.empty() ? "None" : t.callback_ret) + ", "
        + std::to_string(begin) + ", "
        + std::to_string(t.callback_args.size()) + "}");
  }

  std::vector<std::string> fnarg_lits, function_lits;
  for (const auto &f : tree.functions) {
    std::size_t begin = fnarg_lits.size();
    for (const auto &a : f.arg_ctypes)
      fnarg_lits.push_back(qstr(a));
    function_lits.push_back(
        "{" + qstr(f.name) + ", "
        + qstr(f.return_ctype.empty() ? "None" : f.return_ctype) + ", "
        + std::to_string(begin) + ", " + std::to_string(f.arg_ctypes.size())
        + "}");
  }

  std::vector<std::string> const_lits;
  for (const auto &c : tree.constants)
    const_lits.push_back("{" + qstr(c.name) + ", " + qstr(c.value) + "}");

  std::vector<std::string> topo_lits;
  for (auto i : topo)
    topo_lits.push_back(std::to_string(i));

  // ----- emit -----
  auto dump = [&](const char *type, const char *name,
                  const std::vector<std::string> &lits) {
    out << "inline constexpr std::array<" << type << ", " << lits.size() << "> "
        << name << " = ";
    if (lits.empty()) {
      out << "{};\n\n";
      return;
    }
    out << "{{\n";
    for (const auto &l : lits)
      out << "    " << l << ",\n";
    out << "}};\n\n";
  };

  out << "// Generated by webgpu_introspect. Do not edit by hand.\n"
         "// Source: webgpu.h\n"
         "//\n"
         "// Flattened, homogeneous, constexpr-walkable model of the API — the\n"
         "// ycetl-shape \"result memory\": std::array of plain structs, no void*,\n"
         "// no per-N templates. A constexpr consumer can walk all of it; see\n"
         "// webgpu_emit_py.cpp, which rebuilds the Python bindings from it.\n"
         "#pragma once\n"
         "#include <array>\n"
         "#include <cstddef>\n"
         "#include <cstdint>\n"
         "#include <string_view>\n\n"
         "namespace webgpu_tree {\n\n"
         "using std::string_view;\n\n"
         "struct field { string_view name; string_view ctype; string_view src;\n"
         "               std::int64_t offset_bytes; std::int64_t size_bytes; };\n"
         "struct record { string_view name; bool opaque;\n"
         "                std::size_t field_begin; std::size_t field_count; };\n"
         "struct enumerator { string_view name; long long value; bool is_alias; };\n"
         "struct enumeration { string_view name;\n"
         "                     std::size_t value_begin; std::size_t value_count; };\n"
         "struct alias { string_view name; string_view ctype; };\n"
         "struct callback { string_view name; string_view ret;\n"
         "                  std::size_t arg_begin; std::size_t arg_count; };\n"
         "struct function { string_view name; string_view ret;\n"
         "                  std::size_t arg_begin; std::size_t arg_count; };\n"
         "struct constant { string_view name; string_view value; };\n\n";

  out << "inline constexpr std::size_t record_count   = " << tree.records.size()
      << ";\n"
      << "inline constexpr std::size_t enum_count     = " << tree.enums.size()
      << ";\n"
      << "inline constexpr std::size_t typedef_count  = " << tree.typedefs.size()
      << ";\n"
      << "inline constexpr std::size_t function_count = "
      << tree.functions.size() << ";\n\n";

  dump("field", "fields", field_lits);
  dump("record", "records", record_lits);   // name order (struct-stub order)
  dump("std::size_t", "topo", topo_lits);    // _fields_ emission order
  dump("enumerator", "enum_values", enumv_lits);
  dump("enumeration", "enums", enum_lits);
  dump("alias", "aliases", alias_lits);
  dump("string_view", "callback_args", cbarg_lits);
  dump("callback", "callbacks", callback_lits);
  dump("string_view", "function_args", fnarg_lits);
  dump("function", "functions", function_lits);
  dump("constant", "constants", const_lits);

  out << "} // namespace webgpu_tree\n";
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
    std::fprintf(stderr, "usage: webgpu_introspect --header H.h [--python "
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
#ifdef WEBGPU_CLANG_RESOURCE_DIR
  if (std::string_view(WEBGPU_CLANG_RESOURCE_DIR).size() > 0) {
    clang_args.push_back("-resource-dir");
    clang_args.push_back(WEBGPU_CLANG_RESOURCE_DIR);
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
  std::sort(tree.constants.begin(), tree.constants.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });

  std::cerr << "webgpu_introspect: records=" << tree.records.size()
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
