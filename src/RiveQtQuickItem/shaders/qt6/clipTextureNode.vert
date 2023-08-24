// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) in vec2 vertex;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;                     //0
    mat4 tranformMatrix;                //64
};

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out vec2 originalVertex;

void main()
{
    originalVertex = vertex;
    gl_Position = qt_Matrix * tranformMatrix * vec4(vertex, 0.0, 1.0);
}
