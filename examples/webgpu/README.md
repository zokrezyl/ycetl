# webgpu — generating WebGPU bindings, two ways

This example is the worked-out form of the most common production use of
**ycetl**: a C API header *is* a machine-readable description of an API, so you
can walk it at build time and emit bindings from it. It does that for WebGPU
(`webgpu.h`) and produces **the same Python `ctypes` bindings by two
independent paths** — one ordinary, one driven entirely by ycetl at
compile time — then proves the two outputs are byte-identical.

It also ships a runnable payoff: an interactive shadertoy app that drives Dawn
through the generated bindings (`dawn/app/`).

## The pipeline

```
        <impl>/include/webgpu.h          (impl = dawn | native)
                  │  libclang
                  ▼
          webgpu_introspect              the parser/emitter (ordinary C++)
            │                 │
   ┌────────┘                 └───────────┐
   ▼ emit_python                          ▼ emit_tree
 webgpu_ctypes.py                    webgpu_tree.hpp
 (PATH A: direct)                    (a constexpr, void*-free model of the API)
                                          │  #include
                                          ▼
                                    webgpu_emit_py.cpp        ← ycetl, at constexpr
                                          ▼
                                 webgpu_ctypes_from_tree.py
                                 (PATH B: ycetl rebuilds it from the tree)

        cmp PATH A == PATH B   →   byte-identical
```

- **Path A (direct):** `webgpu_introspect` walks the header with libclang and
  writes `webgpu_ctypes.py` directly.
- **Path B (ycetl):** the same tool also writes `webgpu_tree.hpp` — a flattened,
  homogeneous, **`void*`-free** `constexpr` model (`field`/`record`/`enum`/
  `function`/`callback`/`constant`/… as `std::array`s of plain structs).
  `webgpu_emit_py.cpp` `#include`s it and **rebuilds the bindings from the tree
  at constant-evaluation time using ycetl**, then a one-line `main()` writes the
  result out.

### How Path B uses ycetl (the yce machine)

`webgpu_emit_py.cpp` is the canonical compile-time → runtime hand-off:

- **working memory** — a `ycetl::string` (a `multitype_memory`-backed
  `dynamic_array<char>`) accumulates the ~150 KB of Python text *during constant
  evaluation*;
- **result memory** — the string is copied into a `constexpr` `std::array<char,
  CAP>` that survives constant evaluation and is **baked into `.rodata`**;
- `main()` just `fwrite`s that baked blob.

No `void*` anywhere — the tree is walked with its types intact, which is the
whole point ycetl exists to make possible in `constexpr`.

## Files

| File | Role |
|------|------|
| `webgpu_introspect.cpp` | libclang walker + the two emitters (`emit_python`, `emit_tree`) |
| `<impl>/include/webgpu.h` | the bundled header for each implementation (`dawn`, `native`) |
| `webgpu_emit_py.cpp` | the **ycetl** constexpr emitter (Path B) |
| `webgpu_tree_check.cpp` | C++ consumer: `static_assert`s over the tree at compile time |
| `<impl>/webgpu_ctypes_check.py` | Python smoke test (struct sizes / enum values) |
| `common.py` | shared Python helpers (`string_view`, smoke-test body) |
| `dawn/app/` | interactive + headless apps driving Dawn via the generated bindings |

## Building it — separated stages

Off by default (libclang is a heavy dep). Each implementation builds into its
own tree (`build-linux/webgpu-<impl>`); every stage is its own make target,
chained by file dependencies, so invoking a later stage builds the earlier ones
first. `<impl>` is `dawn` or `native`:

| `make webgpu-<impl>-…` | emits |
|------------------------|-------|
| `configure` | the cmake build tree |
| `tool` | `webgpu_introspect` |
| `gen` | `webgpu_ctypes.py` + `webgpu_tree.hpp` |
| `tree-check` | the C++ constexpr consumer |
| `emit` | `webgpu_ctypes_from_tree.py` (Path B, via ycetl) |
| `verify` | byte-diffs Path A vs Path B |
| `test` | runs ctest (tree-check + python smoke) |

```sh
make webgpu-dawn-verify     # dawn:   IDENTICAL — ycetl(tree) == direct(libclang)
make webgpu-native-verify   # native: IDENTICAL — ycetl(tree) == direct(libclang)
```

## Caveat — the `emit` stage is a heavy compile

Path B builds the *entire* ~150 KB Python module as a single `constexpr` value,
so the constexpr evaluator runs over the whole file at compile time. That makes
`webgpu-<impl>-emit` take **~28 s to compile** (one-time) and require
`-fconstexpr-steps=2e9` (set on the `webgpu_emit_py` target; the equivalent GCC
limits are set too). It is `EXCLUDE_FROM_ALL`, so a normal build never pays this
cost — only `make webgpu-<impl>-emit` / `-verify` triggers it, and because the
output is a file target, re-running `verify` with nothing changed is <1 s.

## Adapting to another C API

`webgpu_introspect.cpp` is not WebGPU-specific — the record/enum/typedef/
function/constant walks are libclang generics. Point `-DWEBGPU_IMPL` at a
directory holding a different `include/webgpu.h` (or change the example to your
header), and both paths regenerate. The Python smoke test hardcodes a couple of
WebGPU identities (e.g. `RGBA8Unorm`) — adjust those for a different API.
