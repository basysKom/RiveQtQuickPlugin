/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

