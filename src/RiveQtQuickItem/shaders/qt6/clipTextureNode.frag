// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 originalVertex;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;                     //0
    mat4 tranformMatrix;                //64
};

void main()
{
    fragColor = vec4(0.0);
}
