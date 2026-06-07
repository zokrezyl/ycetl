# SPDX-License-Identifier: MIT

"""Shared smoke-test logic for the generated WebGPU ctypes module.

Each implementation has its own thin check script under <impl>/ that calls
run() with the single identity that differs between headers — the
WGPUTextureFormat_RGBA8Unorm ordinal. Everything asserted here is identical
across implementations (struct sizes, field shapes, enum population), so a
regression in webgpu_introspect surfaces here rather than at the first real
WebGPU call from a downstream binding.

The generated `webgpu_ctypes` module is expected on PYTHONPATH (the CMake
test sets it to the build dir that holds the generated file)."""

import ctypes


def string_view(text, keepalive):
    """Build a WGPUStringView from a Python str (or None).

    WebGPU's modern headers pass every label/code/name as a WGPUStringView
    ({const char* data; size_t length;}) embedded by value — the same layout
    in both the Dawn and native bindings, so this helper serves both.

    ctypes does not keep the backing buffer alive on its own, so the UTF-8
    bytes are appended to `keepalive`; hold that list until the WebGPU call
    that consumes the view has returned."""
    import webgpu_ctypes as w
    view = w.WGPUStringView()
    if text is None:
        view.data = None
        view.length = 0
        return view
    raw = text.encode("utf-8")
    buf = ctypes.create_string_buffer(raw, len(raw))  # exact length, no NUL
    keepalive.append(buf)
    view.data = ctypes.cast(buf, ctypes.c_void_p)
    view.length = len(raw)
    return view


def to_str(view) -> str:
    """Read a WGPUStringView back into a Python str."""
    if not view.data or view.length == 0:
        return ""
    return ctypes.string_at(view.data, view.length).decode("utf-8", "replace")


def run(impl: str, expected_rgba8unorm: int) -> int:
    import webgpu_ctypes as w

    # IntEnum: WGPUTextureFormat is the canonical "did the parse work" probe.
    # The RGBA8Unorm ordinal is the one value that differs between Dawn and
    # the webgpu-native / webgpu-headers numbering, so it's passed in.
    assert int(w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm) == expected_rgba8unorm, \
        f"RGBA8Unorm should be {expected_rgba8unorm} per {impl} webgpu.h"

    # Structure with by-value primitive fields.
    assert ctypes.sizeof(w.WGPUColor) == 32, "WGPUColor is 4×c_double"
    names = [f[0] for f in w.WGPUColor._fields_]
    assert names == ["r", "g", "b", "a"], f"WGPUColor fields = {names}"

    # Structure that exercises typedef'd handles + scalars + nextInChain.
    assert ctypes.sizeof(w.WGPUBindGroupEntry) == 56, \
        f"WGPUBindGroupEntry size = {ctypes.sizeof(w.WGPUBindGroupEntry)}"

    # Population sanity.
    n_enums = sum(
        1 for n in dir(w)
        if isinstance(getattr(w, n), type)
        and issubclass(getattr(w, n), w.IntEnum)
        and getattr(w, n) is not w.IntEnum
    )
    assert n_enums > 40, f"only {n_enums} IntEnums emitted"

    print(f"OK [{impl}] — {n_enums} enums, "
          f"WGPUColor={ctypes.sizeof(w.WGPUColor)}B, "
          f"WGPUBindGroupEntry={ctypes.sizeof(w.WGPUBindGroupEntry)}B")
    return 0
