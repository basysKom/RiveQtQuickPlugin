// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

layout(binding = 1) uniform sampler2D u_texture;

void main()
{
    vec4 finalColor = texture(u_texture, texCoord);
    fragColor = finalColor * qt_Opacity;
}
