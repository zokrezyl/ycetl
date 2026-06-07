#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

"""Headless WebGPU demo: render the plasma shader to an offscreen texture and
write a PNG. Driven through the generated `webgpu_ctypes` Dawn bindings — note
there are no FFI signatures, flag values or callback typedefs in this file:
`webgpu_ctypes.load()`, the `WGPU*Usage_*` constants and the `WGPU*Callback`
types all come from the generator.

Run (uv manages a standalone CPython that loads the system Vulkan loader):
    uv run --managed-python --python 3.12 examples/webgpu/dawn/app/app.py out.png

Env overrides: WEBGPU_LIB (libwebgpu_dawn.so), WEBGPU_BINDINGS (dir with
webgpu_ctypes.py).
"""

import ctypes
import os
import struct
import sys
import zlib
from ctypes import c_void_p
from pathlib import Path

HERE = Path(__file__).resolve().parent
WEBGPU_DIR = HERE.parent.parent
REPO = WEBGPU_DIR.parent.parent
sys.path.insert(0, str(WEBGPU_DIR))
sys.path.insert(0, os.environ.get(
    "WEBGPU_BINDINGS",
    str(REPO / "build-linux" / "webgpu-dawn" / "examples" / "webgpu")))

import common                # noqa: E402
import webgpu_ctypes as w    # noqa: E402

LIB_PATH = os.environ.get("WEBGPU_LIB", str(HERE.parent / "lib" / "libwebgpu_dawn.so"))
PROCESS_EVENTS = w.WGPUCallbackMode.WGPUCallbackMode_AllowProcessEvents


def ptr(obj):
    return ctypes.cast(ctypes.pointer(obj), c_void_p)


def pump(lib, instance, device, predicate):
    for _ in range(100000):
        if predicate():
            return
        if device:
            lib.wgpuDeviceTick(device)
        lib.wgpuInstanceProcessEvents(instance)
    raise RuntimeError("timed out waiting for a WebGPU async callback")


def write_png(path, width, height, rgba):
    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    stride = width * 4
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += rgba[y * stride:(y + 1) * stride]
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        f.write(chunk(b"IEND", b""))


def main(argv):
    width, height, time_value = 512, 512, 2.0
    out_path = argv[1] if len(argv) > 1 else str(HERE / "plasma.png")

    if not Path(LIB_PATH).exists():
        print(f"WebGPU library not found: {LIB_PATH}", file=sys.stderr)
        return 1
    lib = w.load(LIB_PATH)   # every wgpu* function, bound by the generator
    keep = []

    instance = lib.wgpuCreateInstance(None)
    assert instance, "wgpuCreateInstance failed"

    got = {}
    acb = w.WGPURequestAdapterCallback(lambda s, a, m, u1, u2: got.update(adapter=a, amsg=common.to_str(m)))
    ai = w.WGPURequestAdapterCallbackInfo()
    ai.mode = PROCESS_EVENTS
    ai.callback = ctypes.cast(acb, c_void_p)
    keep.append(acb)
    lib.wgpuInstanceRequestAdapter(instance, None, ai)
    pump(lib, instance, None, lambda: "adapter" in got)
    adapter = got.get("adapter")
    if not adapter:
        print(f"no adapter: {got.get('amsg')!r}", file=sys.stderr)
        return 2

    ecb = w.WGPUUncapturedErrorCallback(
        lambda d, t, m, u1, u2: print(f"[device error {t}] {common.to_str(m)}", file=sys.stderr))
    dev_desc = w.WGPUDeviceDescriptor()
    dev_desc.uncapturedErrorCallbackInfo.callback = ctypes.cast(ecb, c_void_p)
    dcb = w.WGPURequestDeviceCallback(lambda s, d, m, u1, u2: got.update(device=d))
    di = w.WGPURequestDeviceCallbackInfo()
    di.mode = PROCESS_EVENTS
    di.callback = ctypes.cast(dcb, c_void_p)
    keep += [ecb, dcb]
    lib.wgpuAdapterRequestDevice(adapter, ptr(dev_desc), di)
    pump(lib, instance, None, lambda: "device" in got)
    device = got["device"]
    queue = lib.wgpuDeviceGetQueue(device)

    fmt = int(w.WGPUTextureFormat.WGPUTextureFormat_RGBA8Unorm)

    wgsl = w.WGPUShaderSourceWGSL()
    wgsl.chain.sType = w.WGPUSType.WGPUSType_ShaderSourceWGSL
    wgsl.code = common.string_view((HERE / "plasma.wgsl").read_text(), keep)
    sm = w.WGPUShaderModuleDescriptor()
    sm.nextInChain = ptr(wgsl)
    keep.append(wgsl)
    shader = lib.wgpuDeviceCreateShaderModule(device, ptr(sm))

    uniform = struct.pack("<ffff", float(width), float(height), time_value, 0.0)
    ubo_desc = w.WGPUBufferDescriptor()
    ubo_desc.usage = w.WGPUBufferUsage_Uniform | w.WGPUBufferUsage_CopyDst
    ubo_desc.size = len(uniform)
    ubo = lib.wgpuDeviceCreateBuffer(device, ptr(ubo_desc))
    lib.wgpuQueueWriteBuffer(queue, ubo, 0, uniform, len(uniform))

    target = w.WGPUColorTargetState()
    target.format = fmt
    target.writeMask = w.WGPUColorWriteMask_All
    frag = w.WGPUFragmentState()
    frag.module = shader
    frag.entryPoint = common.string_view("fs_main", keep)
    frag.targetCount = 1
    frag.targets = ptr(target)
    keep += [target, frag]
    pd = w.WGPURenderPipelineDescriptor()
    pd.vertex.module = shader
    pd.vertex.entryPoint = common.string_view("vs_main", keep)
    pd.primitive.topology = w.WGPUPrimitiveTopology.WGPUPrimitiveTopology_TriangleList
    pd.multisample.count = 1
    pd.multisample.mask = 0xFFFFFFFF
    pd.fragment = ptr(frag)
    pipeline = lib.wgpuDeviceCreateRenderPipeline(device, ptr(pd))

    entry = w.WGPUBindGroupEntry()
    entry.binding, entry.buffer, entry.offset, entry.size = 0, ubo, 0, len(uniform)
    bgd = w.WGPUBindGroupDescriptor()
    bgd.layout = lib.wgpuRenderPipelineGetBindGroupLayout(pipeline, 0)
    bgd.entryCount = 1
    bgd.entries = ptr(entry)
    keep.append(entry)
    bind_group = lib.wgpuDeviceCreateBindGroup(device, ptr(bgd))

    tex_desc = w.WGPUTextureDescriptor()
    tex_desc.usage = w.WGPUTextureUsage_RenderAttachment | w.WGPUTextureUsage_CopySrc
    tex_desc.dimension = w.WGPUTextureDimension.WGPUTextureDimension_2D
    tex_desc.size.width, tex_desc.size.height, tex_desc.size.depthOrArrayLayers = width, height, 1
    tex_desc.format = fmt
    tex_desc.mipLevelCount = 1
    tex_desc.sampleCount = 1
    texture = lib.wgpuDeviceCreateTexture(device, ptr(tex_desc))
    view = lib.wgpuTextureCreateView(texture, None)

    bytes_per_row = ((width * 4 + 255) // 256) * 256
    readback_size = bytes_per_row * height
    rb_desc = w.WGPUBufferDescriptor()
    rb_desc.usage = w.WGPUBufferUsage_MapRead | w.WGPUBufferUsage_CopyDst
    rb_desc.size = readback_size
    readback = lib.wgpuDeviceCreateBuffer(device, ptr(rb_desc))

    encoder = lib.wgpuDeviceCreateCommandEncoder(device, None)
    att = w.WGPURenderPassColorAttachment()
    att.view = view
    att.depthSlice = w.WGPU_DEPTH_SLICE_UNDEFINED
    att.loadOp = w.WGPULoadOp.WGPULoadOp_Clear
    att.storeOp = w.WGPUStoreOp.WGPUStoreOp_Store
    att.clearValue.a = 1.0
    rp = w.WGPURenderPassDescriptor()
    rp.colorAttachmentCount = 1
    rp.colorAttachments = ptr(att)
    rpass = lib.wgpuCommandEncoderBeginRenderPass(encoder, ptr(rp))
    lib.wgpuRenderPassEncoderSetPipeline(rpass, pipeline)
    lib.wgpuRenderPassEncoderSetBindGroup(rpass, 0, bind_group, 0, None)
    lib.wgpuRenderPassEncoderDraw(rpass, 3, 1, 0, 0)
    lib.wgpuRenderPassEncoderEnd(rpass)

    src = w.WGPUTexelCopyTextureInfo()
    src.texture = texture
    src.aspect = w.WGPUTextureAspect.WGPUTextureAspect_All
    dst = w.WGPUTexelCopyBufferInfo()
    dst.buffer = readback
    dst.layout.bytesPerRow = bytes_per_row
    dst.layout.rowsPerImage = height
    copy_size = w.WGPUExtent3D()
    copy_size.width, copy_size.height, copy_size.depthOrArrayLayers = width, height, 1
    lib.wgpuCommandEncoderCopyTextureToBuffer(encoder, ptr(src), ptr(dst), ptr(copy_size))

    cmd = lib.wgpuCommandEncoderFinish(encoder, None)
    lib.wgpuQueueSubmit(queue, 1, (c_void_p * 1)(cmd))

    mcb = w.WGPUBufferMapCallback(lambda status, m, u1, u2: got.update(map_status=status))
    mi = w.WGPUBufferMapCallbackInfo()
    mi.mode = PROCESS_EVENTS
    mi.callback = ctypes.cast(mcb, c_void_p)
    keep.append(mcb)
    lib.wgpuBufferMapAsync(readback, w.WGPUMapMode_Read, 0, readback_size, mi)
    pump(lib, instance, device, lambda: "map_status" in got)
    assert got["map_status"] == int(w.WGPUMapAsyncStatus.WGPUMapAsyncStatus_Success), \
        f"buffer map failed: {got['map_status']}"

    mapped = lib.wgpuBufferGetConstMappedRange(readback, 0, readback_size)
    raw = ctypes.string_at(mapped, readback_size)
    lib.wgpuBufferUnmap(readback)

    row = width * 4
    tight = bytearray()
    for y in range(height):
        tight += raw[y * bytes_per_row: y * bytes_per_row + row]
    write_png(out_path, width, height, bytes(tight))
    print(f"wrote {out_path} ({width}x{height})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
