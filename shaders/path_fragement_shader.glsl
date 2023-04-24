// Copyright (C) 2023 by Jeremias Bosch
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 330 core

uniform vec4 u_color;
uniform bool u_useGradient;
uniform int u_gradientType;

uniform int u_numStops;
uniform vec2 u_stopPositions[20]; //normalized!
uniform vec4 u_stopColors[20];

uniform vec2 u_viewportDimensions;
in vec2 v_texCoord;

in float v_gradientCoord;
in vec2 v_gradientStart;
in vec2 v_gradientEnd;
out vec4 outColor;

uniform sampler2D u_alphaMaskTexture;

uniform bool u_useAlphaMask;

// Gradient related functions
vec4 getGradientColor(float gradientCoord) {
    vec4 gradientColor = u_stopColors[0];

    for (int i = 1; i < u_numStops; ++i) {
        if (gradientCoord <= u_stopPositions[i].x) {
            gradientColor = mix(u_stopColors[i - 1], u_stopColors[i], smoothstep(u_stopPositions[i - 1].x, u_stopPositions[i].x, v_gradientCoord));
            break;
        }
    }

    return gradientColor;
}

void main() {
    vec2 normalizedFragCoord = vec2((gl_FragCoord.x / u_viewportDimensions.x * 0.5) * 2.0,
                                    (gl_FragCoord.y / u_viewportDimensions.y * 0.5) * 2.0);

    vec4 baseColor = u_color;
    if (u_useGradient) {
        vec4 gradientColor = getGradientColor(v_gradientCoord);
        baseColor = gradientColor;
    }

   if (u_useAlphaMask) {
       vec4 maskColor = texture(u_alphaMaskTexture, normalizedFragCoord);
       outColor = vec4(baseColor.rgb, baseColor.a * maskColor.a);
   } else {
       outColor = baseColor;
    }
}
