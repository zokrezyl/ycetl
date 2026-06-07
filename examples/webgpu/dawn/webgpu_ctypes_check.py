# SPDX-License-Identifier: MIT

"""Smoke test for the Dawn-generated WebGPU ctypes module."""

import sys
from pathlib import Path

# common.py lives one level up, in examples/webgpu/.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import common

if __name__ == "__main__":
    sys.exit(common.run("dawn", expected_rgba8unorm=22))
