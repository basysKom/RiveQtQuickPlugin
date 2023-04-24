// Copyright (C) 2023 by Jeremias Bosch
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 330 core
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

uniform bool u_useGradient;
uniform int u_gradientType;

uniform vec2 u_gradientStart;
uniform vec2 u_gradientEnd;

uniform vec2 u_gradientCenter;
uniform vec2 u_gradientFocalPoint;
uniform float u_gradientRadius;
uniform vec2 u_viewportDimensions;

uniform mat4 u_transform;
uniform mat4 u_projection;

out float v_gradientCoord;
out vec2 v_gradientStart;
out vec2 v_gradientEnd;

out vec2 v_texCoord;

void main() {
    if (u_gradientType == 0) { // Linear gradient
        vec2 gradientStart = u_gradientStart;
        vec2 gradientEnd = u_gradientEnd;

        vec2 gradientDir = u_gradientEnd - u_gradientStart;
        float gradientLength = length(gradientDir);
        vec2 normalizedGradientDir = gradientDir / gradientLength;

        if (gradientLength > 0.0) {
            vec2 posDir = a_position - u_gradientStart;
            float posCoord = dot(posDir, normalizedGradientDir);
            v_gradientCoord = posCoord / gradientLength;
            v_gradientCoord = clamp(v_gradientCoord, 0.0, 1.0);
        } else {
            v_gradientCoord = 0.0;
        }

        v_gradientStart = gradientStart;
        v_gradientEnd = gradientEnd;
    } else { // Radial gradient
        vec2 dir = a_position - u_gradientCenter;
        vec2 focalDir = u_gradientFocalPoint - u_gradientCenter;
        float dist = length(dir);
        vec2 normalizedDir = dir / dist;
        vec2 normalizedFocalDir = focalDir / u_gradientRadius;
        float dotProduct = dot(normalizedDir, normalizedFocalDir);
        float angle = acos(dotProduct);
        float normalizedAngle = angle / (2.0 * 3.14159);

        v_gradientCoord = dist / u_gradientRadius;
        v_gradientCoord = clamp(v_gradientCoord, 0.0, 1.0) * cos(normalizedAngle);

        v_gradientStart = u_gradientCenter;
        v_gradientEnd = u_gradientFocalPoint;
    }

    v_texCoord = a_texCoord;
    //v_texCoord = vec2(gl_Position.x, gl_Position.y);// / vec2(u_viewportDimensions);
    gl_Position = u_projection * u_transform * vec4(a_position, 0.0, 1.0);
}

