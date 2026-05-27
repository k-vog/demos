#version 450
// ================================================================================================
// Vertex shader
//
// Build:
//     $ glslc -o vk-cube-vs.spv -fshader-stage=vertex vk-cube-vs.glsl
//
// Changelog:
//     ??/??/????: Initial release
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
};

layout (location = 0) in vec3 v_p;

void main()
{
    gl_Position = mvp * vec4(v_p, 1.0f);
}

