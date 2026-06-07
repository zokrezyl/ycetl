# Worked example: generating a Python WebGPU binding with ycetl

This is a practical walk-through of `examples/webgpu/` — the example
that demonstrates **the most useful real-world thing ycetl is built
for**: walking a C API header at build time and producing both a
runtime-usable binding (Python `ctypes`) and a constexpr C++ mirror of
the same API tree.

For the design rationale behind the machinery, see
[`doc/overview.md`](overview.md). This document stays on the surface:
what the example does, how to build it, how to use the output.

## What you get

Pointed at `examples/webgpu/dawn/include/webgpu.h`, `webgpu` emits two
artefacts:

| Artefact          | What it is                                         | Who consumes it                          |
| ----------------- | -------------------------------------------------- | ---------------------------------------- |
| `webgpu_ctypes.py`  | Pure-Python module of `ctypes.Structure` / `IntEnum` definitions, one per record / enum in the header. | Any Python program that wants to call into a WebGPU shared library — no SWIG, no Cython, no manual FFI wrapping. |
| `webgpu_tree.hpp`   | C++ header re-stating the same tree as `constexpr std::array` initialisers. | A consumer TU that wants to walk the WebGPU API at compile time — generate dispatch tables, validate struct layouts, emit further bindings. |

Both come from a single libclang parse, so they can never drift apart.

## Why this is the canonical pattern

The example is the worked-out form of one specific idea: a C header
already *is* a machine-readable description of an API. You don't have
to hand-write Python bindings for WebGPU — let libclang parse the
header, build a tree, and serialise it in two directions.

The shape generalises:

- **Any C/C++ API you want to call from another language.** Vulkan,
  OpenSSL, ICU, ALSA, your own internal C ABI — all the same flow.
  Swap the header, the generator stays.
- **Any compile-time introspection of an external schema.** Replace
  libclang with a YAML/JSON/IDL parser and the same machine emits a
  constexpr tree of the schema instead of the C API.
- **Any "single source of truth → many language bindings" problem.**
  Add another emitter (Rust `#[repr(C)]`, Zig `extern struct`,
  TypeScript declarations) and the parse is reused.

That's why this example earns its own doc — it's not a toy, it's the
shape of the most common production use of ycetl.

## Building it

`webgpu` is **off by default** because libclang is a heavy
dependency. Turn it on with:

```sh
cmake -S . -B build-linux -G Ninja -DYCETL_BUILD_WEBGPU=ON
cmake --build build-linux
ctest --test-dir build-linux
```

The CMake file auto-discovers libclang from Homebrew's LLVM 20 or a
system `libclang-{18,19,20}`. If it can't find one it prints a warning
and skips the example — the rest of the build is unaffected.

Two tests run on build:

- `webgpu_tree_check` — C++ TU that includes `webgpu_tree.hpp` and runs
  `static_assert`s over the constexpr tree (record count, enum count,
  presence of `WGPUTextureFormat` / `WGPUBindGroupEntry`).
- `webgpu_ctypes_check` — Python script that imports `webgpu_ctypes` and
  asserts well-known identities (`WGPUTextureFormat_RGBA8Unorm == 18`,
  `sizeof(WGPUColor) == 32`, `sizeof(WGPUBindGroupEntry) == 56`).

The outputs land in `build-linux/examples/webgpu/`:

```
webgpu_ctypes.py   # Python ctypes module
webgpu_tree.hpp    # constexpr C++ tree
```

## Using `webgpu_ctypes.py` from Python

The generated module is plain Python — no compiled extension, no
build step for the consumer.

```python
import ctypes
import webgpu_ctypes as w

# Enums are IntEnum subclasses.
fmt = w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm
assert int(fmt) == 18

# Structures are ctypes.Structure with _fields_ populated in order,
# offsets matching the C ABI.
color = w.WGPUColor(r=1.0, g=0.5, b=0.0, a=1.0)
assert ctypes.sizeof(w.WGPUColor) == 32

# Loading the WebGPU shared library is your call — once you have it,
# argtypes / restype come from the same module.
wgpu = ctypes.CDLL("libdawn_native.so")
wgpu.wgpuCreateInstance.argtypes = [ctypes.POINTER(w.WGPUInstanceDescriptor)]
wgpu.wgpuCreateInstance.restype  = w.WGPUInstance
```

That's the full path from a C header to a working Python binding.
Everything you'd usually hand-write — field ordering, integer
sizing, enum values, opaque handle types — is exactly what the
generator emits.

To rebuild after upgrading the header, re-run the build. CMake
re-invokes `webgpu_introspect` automatically whenever
`examples/webgpu/dawn/include/webgpu.h` changes.

## Using `webgpu_tree.hpp` from C++

The C++ side is what makes this an *ycetl* example rather than a
plain code generator. The emitted header looks like:

```cpp
// webgpu_tree.hpp (generated)
namespace webgpu_tree {
    inline constexpr std::array records  = { /* … */ };
    inline constexpr std::array enums    = { /* … */ };
    inline constexpr std::array typedefs = { /* … */ };
    inline constexpr std::array functions = { /* … */ };

    inline constexpr std::size_t record_count   = records.size();
    inline constexpr std::size_t enum_count     = enums.size();
    inline constexpr std::size_t typedef_count  = typedefs.size();
    inline constexpr std::size_t function_count = functions.size();
}
```

A consumer can walk it under `static_assert`:

```cpp
#include "webgpu_tree.hpp"

constexpr int find_enum(std::string_view name) {
    for (std::size_t i = 0; i < webgpu_tree::enums.size(); ++i)
        if (webgpu_tree::enums[i].name == name) return int(i);
    return -1;
}

static_assert(find_enum("WGPUTextureFormat") >= 0);
static_assert(webgpu_tree::record_count > 50);
```

See `examples/webgpu/webgpu_tree_check.cpp` for the full version. The
tree itself uses ycetl-shape result memory: plain `std::array`s of
plain structs, baked into `.rodata`, walkable in constant evaluation.

## Adapting it to your own header

The minimal changes to point `webgpu` at a different C API:

1. Drop the new header somewhere under the source tree.
2. In `examples/webgpu/CMakeLists.txt`, change `_webgpu_header` to
   point at it.
3. Adjust the asserts in `webgpu_tree_check.cpp` /
   `webgpu_ctypes_check.py` to reference stable identities from the
   new API (some enum value you trust, some struct size you can
   look up).

`webgpu_introspect.cpp` doesn't know anything WebGPU-specific — the
record / enum / typedef / function walks are libclang generics. The
"wgpu" in the name is the *example*'s API, not the generator's
scope.

## Pointers to source

| What                                          | Where                                                |
| --------------------------------------------- | ---------------------------------------------------- |
| libclang walk + emitters                      | `examples/webgpu/webgpu_introspect.cpp`             |
| C++ consumer (constexpr tree round-trip)      | `examples/webgpu/webgpu_tree_check.cpp`             |
| Python consumer (smoke test)                  | `examples/webgpu/webgpu_ctypes_check.py`            |
| Build wiring (libclang discovery, codegen)    | `examples/webgpu/CMakeLists.txt`                  |
| The bundled WebGPU header                     | `examples/webgpu/dawn/include/webgpu.h`                |
| Why the constexpr tree looks like that        | [`doc/overview.md`](overview.md)                     |
