// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 originalVertex;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;                     //0
    float qt_Opacity;                   //64
    float gradientRadius;               //68
    int useGradient;                    //72
    int useTexture;                     //76
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
layout(binding = 1) uniform sampler2D image;

vec4 getGradientColor( float gradientCoord) {
    vec4 gradientColor = stopColors[0];

    if (gradientCoord >= gradientPositions[numberOfStops - 1].x) {
        return stopColors[numberOfStops - 1];
    }

    for (int i = 1; i < numberOfStops; ++i) {
        if (gradientCoord <= gradientPositions[i].x) {
            gradientColor = mix(stopColors[i - 1], stopColors[i], smoothstep(gradientPositions[i - 1].x, gradientPositions[i].x, gradientCoord));
            break;
        }
    }
    return gradientColor;
}

void main()
{
    if (useTexture == 1) {
        fragColor = texture(image, texCoord);
        //fragColor = vec4(1,0,0,1);
    } else {
        if (useGradient == 1) {
            float gradientCoord;
            if (gradientType == 0) { // Linear gradient
                vec2 dir = originalVertex - startPoint;
                vec2 gradientDir = endPoint - startPoint;
                gradientCoord = dot(dir, gradientDir) / dot(gradientDir, gradientDir);
            } else { // Radial gradient
                vec2 dir = originalVertex - gradientCenter;
                gradientCoord = length(dir) / gradientRadius;
            }
            gradientCoord = clamp(gradientCoord, 0.0, 1.0);
            fragColor = getGradientColor(gradientCoord);
        } else {
            fragColor = color;
        }
    }
    fragColor = fragColor * qt_Opacity;
}
