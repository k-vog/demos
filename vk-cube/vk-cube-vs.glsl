#version 450
// ================================================================================================
// Vertex shader
//
// Build:
//     $ glslc -o vk-cube-vs.spv -fshader-stage=vertex vk-cube-vs.glsl
//
// Changelog:
//     5/31/2026: Initial release
//
// License:
//     Copyright (c) 2026 Hunter Kvalevog
//
//     Permission to use, copy, modify, and/or distribute this software for any
//     purpose with or without fee is hereby granted.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//     WITH REGARD TO THIS SOFTWARE.
// ================================================================================================

layout (binding = 0) uniform UBO
{
    mat4 mvp;
    mat4 model;
};

layout (location = 0) in vec3 v_p;
layout (location = 1) in vec3 v_c;
layout (location = 2) in vec3 v_n;

layout (location = 0) out vec3 f_c;
layout (location = 1) out vec3 f_n;
layout (location = 2) out vec3 f_p;

void main()
{
    f_c = v_c;
    f_n = mat3(model) * v_n;
    f_p = (model * vec4(v_p, 1.0f)).xyz;
    gl_Position = mvp * vec4(v_p, 1.0f);
}

