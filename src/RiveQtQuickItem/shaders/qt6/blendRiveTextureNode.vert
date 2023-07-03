// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) in vec2 vertex;
layout(location = 1) in vec2 aTexCoord; // Add texture coordinates

layout(location = 0) out vec2 texCoord; // Pass texture coordinates to the fragment shader

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    int flipped;
};

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    if (flipped == 0) {
        texCoord = aTexCoord;
    } else {
        texCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y); // Pass the texture coordinates to the fragment shader
    }

    gl_Position = qt_Matrix * vec4(vertex, 0.0, 1.0);
}
