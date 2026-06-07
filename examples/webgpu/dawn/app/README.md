# Dawn shader demo

Drives **Dawn** (the loadable `libwebgpu_dawn.so`) from Python through the
generated `webgpu_ctypes` bindings.

```
interactive.py  animated shadertoy plasma in a window (the main demo)
app.py          headless variant: renders one frame to a PNG
plasma.wgsl     the shadertoy-style fragment shader (adapted from yetty)
```

## Running

`interactive.py` is a [uv](https://docs.astral.sh/uv/) script: the inline
`# /// script` header declares its one dependency (`glfw`), and the shebang
runs it on a uv-managed standalone CPython — which loads the system Vulkan
loader and X11 libs cleanly (no `LD_LIBRARY_PATH`/ICD setup).

```sh
examples/webgpu/dawn/app/interactive.py        # opens a window, Esc to quit
uv run examples/webgpu/dawn/app/interactive.py # equivalent
```

Headless PNG (no window):

```sh
uv run --managed-python --python 3.12 examples/webgpu/dawn/app/app.py out.png
```

Env overrides: `WEBGPU_LIB` (path to `libwebgpu_dawn.so`), `WEBGPU_BINDINGS`
(dir with the generated `webgpu_ctypes.py`), `WEBGPU_FRAMES` (render N frames
then exit — used by smoke tests).

## What makes it work

The generated ctypes structs are **ABI-correct** — embedded `WGPUStringView`
labels and `WGPUChainedStruct` chains have their true layout, and enums are
4-byte. String labels/entry-points come from `common.string_view()`.

The header and the library **must be the same Dawn version** (enum values like
`WGPUTextureFormat_RGBA8Unorm` moved between versions). `dawn/include/webgpu.h`
is synced to the same release that produced the `.so`.

## Getting the library

The repo does not vendor the 19 MB `.so`. Build it from a Dawn release
(static archive → shared object) and sync the matching header:

```sh
# from a Dawn release tarball (lib64/libwebgpu_dawn.a + include/dawn/webgpu.h):
printf '{ global: wgpu*; local: *; };\n' > /tmp/wgpu.map
g++ -shared -fPIC -o examples/webgpu/dawn/lib/libwebgpu_dawn.so \
    -Wl,--whole-archive <dist>/lib64/libwebgpu_dawn.a -Wl,--no-whole-archive \
    -Wl,--version-script=/tmp/wgpu.map -lpthread -ldl -lm
cp <dist>/include/dawn/webgpu.h examples/webgpu/dawn/include/webgpu.h
make webgpu-dawn-gen
```
