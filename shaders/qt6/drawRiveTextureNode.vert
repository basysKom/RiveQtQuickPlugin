// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) in vec2 vertex;
layout(location = 1) in vec2 aTexCoord; // Add texture coordinates

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;                     //0
    float qt_Opacity;                   //64
    float gradientRadius;               //68
    int useGradient;                    //72
    int blendMode;                      //76
    vec2 gradientFocalPoint;            //80
    vec2 gradientCenter;                //88
    vec2 startPoint;                    //96
    vec2 endPoint;                      //104
    int numberOfStops;                  //112
    int gradientType;                   //116
    vec4 color;                         //128
    vec4 stopColors[20];                //144
    vec2 gradientPositions[20];         //464
    mat4 tranformMatrix;                //784
};

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out vec2 texCoord; // Pass texture coordinates to the fragment shader
layout(location = 1) out vec2 originalVertex;

void main()
{
    texCoord = aTexCoord;
    originalVertex = vertex;

    gl_Position = qt_Matrix * tranformMatrix * vec4(vertex, 0.0, 1.0);
}
