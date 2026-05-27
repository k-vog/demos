#version 450
// ================================================================================================
// Fragment shader
//
// Build:
//     $ glslc -o vk-cube-fs.spv -fshader-stage=fragment vk-cube-fs.glsl
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

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

