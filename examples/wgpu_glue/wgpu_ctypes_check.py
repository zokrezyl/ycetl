"""End-to-end smoke test for the generated WebGPU ctypes module.

Asserts a handful of stable identities (well-known enum values, struct
sizes, field shapes) so a regression in wgpu_introspect surfaces here
rather than at the first real WebGPU call from a downstream binding."""

import ctypes
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import wgpu_ctypes as w


def main() -> int:
    # IntEnum: WGPUTextureFormat is the canonical "did the parse work" probe.
    assert int(w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm) == 18, \
        "RGBA8Unorm should be 18 per dawn/webgpu.h"

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

    print(f"OK — {n_enums} enums, "
          f"WGPUColor={ctypes.sizeof(w.WGPUColor)}B, "
          f"WGPUBindGroupEntry={ctypes.sizeof(w.WGPUBindGroupEntry)}B")
    return 0


if __name__ == "__main__":
    sys.exit(main())
