#!/usr/bin/env -S uv run --script --managed-python --python 3.12
# SPDX-License-Identifier: MIT
# /// script
# requires-python = ">=3.11"
# dependencies = ["glfw"]
# ///

"""Interactive WebGPU demo: an animated shadertoy plasma in a window.

Driven entirely through the generated `webgpu_ctypes` Dawn bindings. Note what
is NOT in this file: no FFI signatures, no flag values, no callback typedefs —
`webgpu_ctypes.load()`, the `WGPU*Usage_*` constants and the `WGPU*Callback`
types all come straight from the generator. The app just builds descriptors
and runs a render loop. Press Esc or close the window to quit.

Run with uv (it manages the interpreter + the `glfw` dependency on a standalone
CPython that loads the system Vulkan loader / X11 libs cleanly):

    examples/webgpu/dawn/app/interactive.py

Env overrides: WEBGPU_LIB (libwebgpu_dawn.so), WEBGPU_BINDINGS (dir with
webgpu_ctypes.py), WEBGPU_FRAMES (render N frames then exit; for smoke tests).
"""

import ctypes
import os
import struct
import sys
import time
from ctypes import c_void_p
from pathlib import Path

import glfw

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


def ptr(obj):
    return ctypes.cast(ctypes.pointer(obj), c_void_p)


def pump(lib, instance, predicate):
    for _ in range(100000):
        if predicate():
            return
        lib.wgpuInstanceProcessEvents(instance)
    raise RuntimeError("timed out waiting for a WebGPU callback")


def make_surface(lib, instance, window, keep):
    """Build a WGPUSurface from the GLFW window's native handle."""
    if glfw.get_platform() == glfw.PLATFORM_WAYLAND:
        src = w.WGPUSurfaceSourceWaylandSurface()
        src.chain.sType = w.WGPUSType.WGPUSType_SurfaceSourceWaylandSurface
        src.display = glfw.get_wayland_display()
        src.surface = glfw.get_wayland_window(window)
    else:
        src = w.WGPUSurfaceSourceXlibWindow()
        src.chain.sType = w.WGPUSType.WGPUSType_SurfaceSourceXlibWindow
        src.display = glfw.get_x11_display()
        src.window = glfw.get_x11_window(window)
    desc = w.WGPUSurfaceDescriptor()
    desc.nextInChain = ptr(src)
    keep += [src, desc]
    return lib.wgpuInstanceCreateSurface(instance, ptr(desc))


def main():
    if not Path(LIB_PATH).exists():
        print(f"WebGPU library not found: {LIB_PATH}", file=sys.stderr)
        return 1
    lib = w.load(LIB_PATH)   # every wgpu* function, bound by the generator
    keep = []

    if not glfw.init():
        print("glfw init failed (no display?)", file=sys.stderr)
        return 1
    glfw.window_hint(glfw.CLIENT_API, glfw.NO_API)
    window = glfw.create_window(800, 600, "webgpu plasma — Dawn via ctypes", None, None)
    if not window:
        glfw.terminate()
        print("window creation failed", file=sys.stderr)
        return 1

    instance = lib.wgpuCreateInstance(None)
    surface = make_surface(lib, instance, window, keep)
    assert surface, "surface creation failed"

    got = {}
    acb = w.WGPURequestAdapterCallback(lambda s, a, m, u1, u2: got.update(adapter=a))
    opts = w.WGPURequestAdapterOptions()
    opts.compatibleSurface = surface
    ai = w.WGPURequestAdapterCallbackInfo()
    ai.mode = w.WGPUCallbackMode.WGPUCallbackMode_AllowProcessEvents
    ai.callback = ctypes.cast(acb, c_void_p)
    keep.append(acb)
    lib.wgpuInstanceRequestAdapter(instance, ptr(opts), ai)
    pump(lib, instance, lambda: "adapter" in got)
    adapter = got.get("adapter")
    assert adapter, "no adapter"

    ecb = w.WGPUUncapturedErrorCallback(
        lambda d, t, m, u1, u2: print(f"[device error {t}] {common.to_str(m)}", file=sys.stderr))
    dev_desc = w.WGPUDeviceDescriptor()
    dev_desc.uncapturedErrorCallbackInfo.callback = ctypes.cast(ecb, c_void_p)
    dcb = w.WGPURequestDeviceCallback(lambda s, d, m, u1, u2: got.update(device=d))
    di = w.WGPURequestDeviceCallbackInfo()
    di.mode = w.WGPUCallbackMode.WGPUCallbackMode_AllowProcessEvents
    di.callback = ctypes.cast(dcb, c_void_p)
    keep += [ecb, dcb]
    lib.wgpuAdapterRequestDevice(adapter, ptr(dev_desc), di)
    pump(lib, instance, lambda: "device" in got)
    device = got.get("device")
    assert device, "no device"
    queue = lib.wgpuDeviceGetQueue(device)

    # Pick a surface format the adapter supports (prefer BGRA8Unorm).
    caps = w.WGPUSurfaceCapabilities()
    lib.wgpuSurfaceGetCapabilities(surface, adapter, ptr(caps))
    bgra = int(w.WGPUTextureFormat.WGPUTextureFormat_BGRA8Unorm)
    surface_format = bgra
    if caps.formatCount:
        formats = ctypes.cast(caps.formats, ctypes.POINTER(ctypes.c_int32))
        available = [formats[i] for i in range(caps.formatCount)]
        surface_format = bgra if bgra in available else available[0]

    # Shader + pipeline targeting the surface format.
    wgsl = w.WGPUShaderSourceWGSL()
    wgsl.chain.sType = w.WGPUSType.WGPUSType_ShaderSourceWGSL
    wgsl.code = common.string_view((HERE / "plasma.wgsl").read_text(), keep)
    sm = w.WGPUShaderModuleDescriptor()
    sm.nextInChain = ptr(wgsl)
    keep.append(wgsl)
    shader = lib.wgpuDeviceCreateShaderModule(device, ptr(sm))
    assert shader, "shader failed"

    target = w.WGPUColorTargetState()
    target.format = surface_format
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
    assert pipeline, "pipeline failed"

    # Uniform buffer + bind group.
    ubo_desc = w.WGPUBufferDescriptor()
    ubo_desc.usage = w.WGPUBufferUsage_Uniform | w.WGPUBufferUsage_CopyDst
    ubo_desc.size = 16
    ubo = lib.wgpuDeviceCreateBuffer(device, ptr(ubo_desc))
    entry = w.WGPUBindGroupEntry()
    entry.binding, entry.buffer, entry.offset, entry.size = 0, ubo, 0, 16
    bgd = w.WGPUBindGroupDescriptor()
    bgd.layout = lib.wgpuRenderPipelineGetBindGroupLayout(pipeline, 0)
    bgd.entryCount = 1
    bgd.entries = ptr(entry)
    keep.append(entry)
    bind_group = lib.wgpuDeviceCreateBindGroup(device, ptr(bgd))

    def configure(width, height):
        cfg = w.WGPUSurfaceConfiguration()
        cfg.device = device
        cfg.format = surface_format
        cfg.usage = w.WGPUTextureUsage_RenderAttachment
        cfg.width, cfg.height = width, height
        cfg.alphaMode = w.WGPUCompositeAlphaMode.WGPUCompositeAlphaMode_Auto
        cfg.presentMode = w.WGPUPresentMode.WGPUPresentMode_Fifo
        lib.wgpuSurfaceConfigure(surface, ptr(cfg))

    fb_w, fb_h = glfw.get_framebuffer_size(window)
    configure(fb_w, fb_h)

    max_frames = int(os.environ.get("WEBGPU_FRAMES", "0"))
    start = time.monotonic()
    frame = 0
    print(f"running: {fb_w}x{fb_h}, surface_format={surface_format}. Esc to quit.")
    while not glfw.window_should_close(window):
        glfw.poll_events()
        if glfw.get_key(window, glfw.KEY_ESCAPE) == glfw.PRESS:
            break
        nw, nh = glfw.get_framebuffer_size(window)
        if (nw, nh) != (fb_w, fb_h) and nw > 0 and nh > 0:
            fb_w, fb_h = nw, nh
            configure(fb_w, fb_h)

        lib.wgpuQueueWriteBuffer(queue, ubo, 0,
                                 struct.pack("<ffff", float(fb_w), float(fb_h),
                                             time.monotonic() - start, 0.0), 16)

        st = w.WGPUSurfaceTexture()
        lib.wgpuSurfaceGetCurrentTexture(surface, ptr(st))
        if not st.texture:
            continue
        view = lib.wgpuTextureCreateView(st.texture, None)

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
        cmd = lib.wgpuCommandEncoderFinish(encoder, None)
        lib.wgpuQueueSubmit(queue, 1, (c_void_p * 1)(cmd))
        lib.wgpuSurfacePresent(surface)

        for release, handle in (
                (lib.wgpuCommandBufferRelease, cmd),
                (lib.wgpuRenderPassEncoderRelease, rpass),
                (lib.wgpuCommandEncoderRelease, encoder),
                (lib.wgpuTextureViewRelease, view),
                (lib.wgpuTextureRelease, st.texture)):
            release(handle)
        lib.wgpuDeviceTick(device)

        frame += 1
        if max_frames and frame >= max_frames:
            break

    print(f"rendered {frame} frames")
    glfw.terminate()
    return 0


if __name__ == "__main__":
    sys.exit(main())
