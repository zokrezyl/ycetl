// Shadertoy-style plasma — adapted from yetty's demo plasma shader
// (src/yetty/yshadertoy/demo-shaders.c). Self-contained: no textures, no
// vertex buffer. A 3-vertex oversized triangle covers the viewport, and the
// fragment shader animates a sine-interference plasma from a time uniform.

struct Uniforms {
    resolution : vec2<f32>,  // viewport size in pixels
    time       : f32,        // seconds since start
    _pad       : f32,        // std140-friendly 16-byte alignment
};

@group(0) @binding(0) var<uniform> u : Uniforms;

struct VSOut {
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,  // 0..1 across the viewport
};

@vertex
fn vs_main(@builtin(vertex_index) vid : u32) -> VSOut {
    // Fullscreen triangle: covers [-1,1]^2 with a single oversized tri.
    var corners = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0),
    );
    let xy = corners[vid];
    var out : VSOut;
    out.position = vec4<f32>(xy, 0.0, 1.0);
    out.uv = xy * 0.5 + 0.5;
    return out;
}

fn plasma(frag_coord : vec2<f32>, time : f32) -> vec3<f32> {
    let p = frag_coord * 0.02;
    let t = time * 0.5;
    var v = 0.0;
    v += sin(p.x + t);
    v += sin(p.y + t * 0.5);
    v += sin(p.x + p.y + t * 0.3);
    v += sin(sqrt(p.x * p.x + p.y * p.y) * 0.5 + t);
    v = v / 4.0;
    let PI = 3.14159265;
    return vec3<f32>(
        sin(v * PI) * 0.5 + 0.5,
        sin(v * PI + 2.094) * 0.5 + 0.5,
        sin(v * PI + 4.188) * 0.5 + 0.5,
    );
}

@fragment
fn fs_main(in : VSOut) -> @location(0) vec4<f32> {
    let frag_coord = in.uv * u.resolution;
    return vec4<f32>(plasma(frag_coord, u.time), 1.0);
}
