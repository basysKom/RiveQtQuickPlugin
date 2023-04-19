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

in vec2 v_texCoord;

out vec4 FragColor;

uniform sampler2D u_blendTexture;
uniform sampler2D u_displayTexture;
uniform int u_blendMode;
uniform float u_blendFactor;

// Blend Mode functions
vec4 overlay(vec4 srcColor, vec4 destColor) {

    // check why this is actually need to get a good result
    srcColor.rgb *= 2.0;

    float ra = (destColor.r < 0.5) ? (2.0 * destColor.r * srcColor.r) : (1.0 - 2.0 * (1.0 - destColor.r) * (1.0 - srcColor.r));
    float ga = (destColor.g < 0.5) ? (2.0 * destColor.g * srcColor.g) : (1.0 - 2.0 * (1.0 - destColor.g) * (1.0 - srcColor.g));
    float ba = (destColor.b < 0.5) ? (2.0 * destColor.b * srcColor.b) : (1.0 - 2.0 * (1.0 - destColor.b) * (1.0 - srcColor.b));
    float aa = srcColor.a + destColor.a * (1.0 - srcColor.a);

    return vec4(ra, ga, ba, srcColor.a );
}

vec4 colorDodge(vec4 srcColor, vec4 destColor) {
    vec4 result = vec4(1.0) - (vec4(1.0) - destColor) / srcColor;
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

vec4 colorBurn(vec4 srcColor, vec4 destColor) {
    vec4 result = vec4(1.0) - (vec4(1.0) - destColor) / srcColor;
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

vec4 hardLight(vec4 srcColor, vec4 destColor) {
    vec4 result;
    result.r = (srcColor.r < 0.5) ? (2.0 * srcColor.r * destColor.r) : (1.0 - 2.0 * (1.0 - srcColor.r) * (1.0 - destColor.r));
    result.g = (srcColor.g < 0.5) ? (2.0 * srcColor.g * destColor.g) : (1.0 - 2.0 * (1.0 - srcColor.g) * (1.0 - destColor.g));
    result.b = (srcColor.b < 0.5) ? (2.0 * srcColor.b * destColor.b) : (1.0 - 2.0 * (1.0 - srcColor.b) * (1.0 - destColor.b));
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

vec4 softLight(vec4 srcColor, vec4 destColor) {
    vec4 result;
    result.r = (srcColor.r < 0.5) ? (2.0 * srcColor.r * destColor.r + destColor.r * destColor.r * (1.0 - 2.0 * srcColor.r)) : (2.0 * srcColor.r * (1.0 - destColor.r) + sqrt(destColor.r) * ((2.0 * srcColor.r) - 1.0));
    result.g = (srcColor.g < 0.5) ? (2.0 * srcColor.g * destColor.g + destColor.g * destColor.g * (1.0 - 2.0 * srcColor.g)) : (2.0 * srcColor.g * (1.0 - destColor.g) + sqrt(destColor.g) * ((2.0 * srcColor.g) - 1.0));
    result.b = (srcColor.b < 0.5) ? (2.0 * srcColor.b * destColor.b + destColor.b * destColor.b * (1.0 - 2.0 * srcColor.b)) : (2.0 * srcColor.b * (1.0 - destColor.b) + sqrt(destColor.b) * ((2.0 * srcColor.b) - 1.0));
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

vec4 exclusion(vec4 srcColor, vec4 destColor) {
    vec4 result = srcColor + destColor - 2.0 * srcColor * destColor;
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

vec3 rgb2hsl(vec3 color) {
    float maxColor = max(max(color.r, color.g), color.b);
    float minColor = min(min(color.r, color.g), color.b);
    float delta = maxColor - minColor;

    vec3 hsl = vec3(0.0, 0.0, (maxColor + minColor) / 2.0);

    if (delta != 0.0) {
        hsl.y = (hsl.z < 0.5) ? (delta / (maxColor + minColor)) : (delta / (2.0 - maxColor - minColor));

        if (maxColor == color.r) {
            hsl.x = (color.g - color.b) / delta + (color.g < color.b ? 6.0 : 0.0);
        } else if (maxColor == color.g) {
            hsl.x = (color.b - color.r) / delta + 2.0;
        } else {
            hsl.x = (color.r - color.g) / delta + 4.0;
        }

        hsl.x /= 6.0;
    }

    return hsl;
}

float hue2rgb(float p, float q, float t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

vec3 hsl2rgb(vec3 hsl) {
    if (hsl.y == 0.0) {
        return vec3(hsl.z);
    } else {
        float q = hsl.z < 0.5 ? hsl.z * (1.0 + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
        float p = 2.0 * hsl.z - q;
        return vec3(hue2rgb(p, q, hsl.x + 1.0 / 3.0), hue2rgb(p, q, hsl.x), hue2rgb(p, q, hsl.x - 1.0 / 3.0));
    }
}

vec4 hue(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(srcHSL.x, destHSL.yz));
    float resultA = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return vec4(resultRGB, resultA);
}

vec4 saturation(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(destHSL.x, srcHSL.y, destHSL.z));
    float resultA = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return vec4(resultRGB, resultA);
}

vec4 color(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(srcHSL.xy, destHSL.z));
    float resultA = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return vec4(resultRGB, resultA);
}

vec4 luminosity(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(destHSL.xy, srcHSL.z));
    float resultA = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return vec4(resultRGB, resultA);
}

vec4 screen(vec4 srcColor, vec4 destColor) {
    vec4 result = destColor + srcColor - (destColor * srcColor);
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

void main()
{
    vec4 srcColor = texture(u_blendTexture, v_texCoord);
    vec4 destColor = texture(u_displayTexture, v_texCoord);

    if (u_blendMode == 14) {
        FragColor = screen(srcColor, destColor);
    } else if (u_blendMode == 15) { // 15 corresponds to the overlay blend mode
        FragColor = overlay(srcColor, destColor);
    } else if (u_blendMode == 18) { // 18 corresponds to the color dodge blend mode
        FragColor = colorDodge(srcColor, destColor);
    } else if (u_blendMode == 19) { // 19 corresponds to the color burn blend mode
        FragColor = colorBurn(srcColor, destColor);
    } else if (u_blendMode == 20) { // 20 corresponds to the hard light blend mode
        FragColor = hardLight(srcColor, destColor);
    } else if (u_blendMode == 21) { // 21 corresponds to the soft light blend mode
        FragColor = softLight(srcColor, destColor);
    } else if (u_blendMode == 22) { // 22 corresponds to the exclusion blend mode
        FragColor = exclusion(srcColor, destColor);
    } else if (u_blendMode == 23) { // 23 corresponds to the hue blend mode
        FragColor = hue(srcColor, destColor);
    } else if (u_blendMode == 24) { // 24 corresponds to the saturation blend mode
        FragColor = saturation(srcColor, destColor);
    } else if (u_blendMode == 25) { // 25 corresponds to the color blend mode
        FragColor = color(srcColor, destColor);
    } else if (u_blendMode == 26) { // 26 corresponds to the luminosity blend mode
        FragColor = luminosity(srcColor, destColor);
    } else {
        // default case, if no blend mode is specified, just output the source color
        FragColor = destColor;
    }
}
