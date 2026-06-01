#version 450
// ================================================================================================
// Fragment shader, basic Phong shading
//
// Build:
//     $ glslc -o vk-cube-fs.spv -fshader-stage=fragment vk-cube-fs.glsl
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

layout (location = 0) in vec3 f_c;
layout (location = 1) in vec3 f_n;
layout (location = 2) in vec3 f_p;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 light_pos = vec3(0.0f, 0.0f, -2.0f);
    vec3 light_dir = normalize(light_pos - f_p);
    vec3 view_dir  = normalize(-f_p);

    // ambient
    float ambient = 0.15f;

    // diffuse
    float diffuse = max(dot(f_n, light_dir), 0.0f);

    // specular
    vec3 reflect_dir = reflect(-light_dir, f_n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), 32.0f);

    vec3 result = (ambient + diffuse + 0.5 * spec) * f_c;

    out_color = vec4(result, 1.0f);
}

