// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    int blendMode;
};

layout(binding = 1) uniform sampler2D u_texture_src;
layout(binding = 2) uniform sampler2D u_texture_dest;

vec4 overlay(vec4 srcColor, vec4 destColor) {
    // not perfect matching the skia overlay blend still some visual differences...
    float luminance = dot(srcColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f));

    vec3 blendedColor = mix(
        2.0f * srcColor.rgb * destColor.rgb,
        1.0f - 2.0f * (1.0f - srcColor.rgb) * (1.0f - destColor.rgb),
        step(0.5f, luminance)
      );

    blendedColor = blendedColor * (destColor.a) + srcColor.rgb * ( 1.0f - destColor.a);

    return vec4(blendedColor, 1);
}

// ok
vec4 colorDodge(vec4 destColor, vec4 srcColor) {
    vec3 result;

    // Apply the color dodge operation to each color channel
    result.r = (srcColor.r == 1.0) ? 1.0 : min(1.0, destColor.r / (1.0 - srcColor.r));
    result.g = (srcColor.g == 1.0) ? 1.0 : min(1.0, destColor.g / (1.0 - srcColor.g));
    result.b = (srcColor.b == 1.0) ? 1.0 : min(1.0, destColor.b / (1.0 - srcColor.b));

    // Blend the alpha values
    float resultA = srcColor.a + destColor.a * (1.0 - srcColor.a);

    // Mix the source and result colors based on the source alpha
    vec3 finalColor = mix(destColor.rgb, result, srcColor.a);

    return vec4(finalColor, resultA);
}

// ok
vec4 colorBurn(vec4 destColor, vec4 srcColor) {
    vec3 result;

    // Apply the color burn operation to each color channel
    result.r = (srcColor.r == 0.0) ? 0.0 : max(0.0, 1.0 - (1.0 - destColor.r) / srcColor.r);
    result.g = (srcColor.g == 0.0) ? 0.0 : max(0.0, 1.0 - (1.0 - destColor.g) / srcColor.g);
    result.b = (srcColor.b == 0.0) ? 0.0 : max(0.0, 1.0 - (1.0 - destColor.b) / srcColor.b);

    // Blend the alpha values
    float resultA = srcColor.a + destColor.a * (1.0 - srcColor.a);

    // Mix the source and result colors based on the source alpha
    vec3 finalColor = mix(destColor.rgb, result, srcColor.a);

    return vec4(finalColor, resultA);
}

//ok
vec4 hardLight(vec4 srcColor, vec4 destColor) {

    if (destColor.a == 0) return srcColor;

    vec4 result;
    result.r = (srcColor.r < 0.5) ? (2.0 * srcColor.r * destColor.r) : (1.0 - 2.0 * (1.0 - srcColor.r) * (1.0 - destColor.r));
    result.g = (srcColor.g < 0.5) ? (2.0 * srcColor.g * destColor.g) : (1.0 - 2.0 * (1.0 - srcColor.g) * (1.0 - destColor.g));
    result.b = (srcColor.b < 0.5) ? (2.0 * srcColor.b * destColor.b) : (1.0 - 2.0 * (1.0 - srcColor.b) * (1.0 - destColor.b));
    result.a = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return result;
}

float D(float Cb) {
    return (Cb <= 0.25) ? ((16.0 * Cb - 12.0) * Cb + 4.0) * Cb : sqrt(Cb);
}

//ok
vec4 softLight(vec4 srcColor, vec4 destColor) {
    vec4 result;
    result.r = (destColor.r <= 0.5) ? srcColor.r - (1.0 - 2.0 * destColor.r) * srcColor.r * (1.0 - srcColor.r) : srcColor.r + (2.0 * destColor.r - 1.0) * (D(srcColor.r) - srcColor.r);
    result.g = (destColor.g <= 0.5) ? srcColor.g - (1.0 - 2.0 * destColor.g) * srcColor.g * (1.0 - srcColor.g) : srcColor.g + (2.0 * destColor.g - 1.0) * (D(srcColor.g) - srcColor.g);
    result.b = (destColor.b <= 0.5) ? srcColor.b - (1.0 - 2.0 * destColor.b) * srcColor.b * (1.0 - srcColor.b) : srcColor.b + (2.0 * destColor.b - 1.0) * (D(srcColor.b) - srcColor.b);
    result.a = destColor.a + srcColor.a - destColor.a * srcColor.a;
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
    float r, g, b;

    if (hsl.y == 0.0) {
        r = g = b = hsl.z; // achromatic
    } else {
        float q = hsl.z < 0.5 ? hsl.z * (1.0 + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
        float p = 2.0 * hsl.z - q;

        r = hue2rgb(p, q, hsl.x + 1.0 / 3.0);
        g = hue2rgb(p, q, hsl.x);
        b = hue2rgb(p, q, hsl.x - 1.0 / 3.0);
    }

    return vec3(r, g, b);
}

vec4 hue(vec4 destColor, vec4 srcColor) {
    vec3 destHsl = rgb2hsl(destColor.rgb);
    vec3 srcHsl = rgb2hsl(srcColor.rgb);

    // Take the hue from the source, keep saturation and lightness from the destination
    vec3 resultRgb = hsl2rgb(vec3(srcHsl.x, destHsl.yz));

    // Blend the alpha values
    float resultA = srcColor.a + destColor.a * (1.0 - srcColor.a);

    // Mix the source and result colors based on the source alpha
    vec3 finalColor = mix(destColor.rgb, resultRgb, srcColor.a);

    return vec4(finalColor, resultA);
}

vec4 saturation(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(srcHSL.x, destHSL.y, srcHSL.z));
    float resultA = srcColor.a + destColor.a - srcColor.a * destColor.a;
    return vec4(resultRGB, resultA);
}

vec4 color(vec4 srcColor, vec4 destColor) {
    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(destHSL.xy, srcHSL.z));
    float resultA = destColor.a + srcColor.a - destColor.a * srcColor.a;
    return vec4(resultRGB, resultA);
}


vec4 luminosity(vec4 srcColor, vec4 destColor) {
    if (destColor.a == 0.0) {
        return srcColor;
    }

    vec3 srcHSL = rgb2hsl(srcColor.rgb);
    vec3 destHSL = rgb2hsl(destColor.rgb);
    vec3 resultRGB = hsl2rgb(vec3(srcHSL.x, srcHSL.y, destHSL.z));

    // Calculate alpha blending
    float srcA = srcColor.a;
    float destA = destColor.a;
    float resultA = srcA + destA * (1.0 - srcA);

    // Calculate final color with respect to alpha channel
    vec3 resultColor = (resultRGB * srcA + destColor.rgb * destA * (1.0 - srcA)) / resultA;

    return vec4(resultColor, resultA);
}

//ok
vec4 screen(vec4 srcColor, vec4 destColor) {
    vec4 result;
    result.rgb = 1.0 - (1.0 - destColor.rgb * (1.0f - srcColor.a)) * (1.0 - srcColor.rgb * (1.0f + destColor.a));
    result.a = max(srcColor.a, destColor.a);
    return result;
}


vec4 blend(vec4 srcColor, vec4 destColor, int blendMode) {

    // shortcut, if the layer is transparent, just take the src
    if (destColor.a == 0.0) {
        return srcColor;
    }

    vec4 blendedColor;
    if (blendMode == 0) {          // unused layer! do not blend
        blendedColor = srcColor;
    } else if (blendMode == 14) {
        blendedColor = screen(srcColor, destColor);
    } else if (blendMode == 15) { // 15 corresponds to the overlay blend mode
        blendedColor = overlay(srcColor, destColor);
    } else if (blendMode == 18) { // 18 corresponds to the color dodge blend mode
        blendedColor = colorDodge(srcColor, destColor);
    } else if (blendMode == 19) { // 19 corresponds to the color burn blend mode
        blendedColor = colorBurn(srcColor, destColor);
    } else if (blendMode == 20) { // 20 corresponds to the hard light blend mode
        blendedColor = hardLight(srcColor, destColor);
    } else if (blendMode == 21) { // 21 corresponds to the soft light blend mode
        blendedColor = softLight(srcColor, destColor);
    } else if (blendMode == 23) { // 23 corresponds to the exclusion blend mode
        blendedColor = exclusion(srcColor, destColor);
    } else if (blendMode == 25) { // 25 corresponds to the hue blend mode
        blendedColor = hue(srcColor, destColor);
    } else if (blendMode == 26) { // 26 corresponds to the saturation blend mode
        blendedColor = saturation(srcColor, destColor);
    } else if (blendMode == 27) { // 27 corresponds to the color blend mode
        blendedColor = color(srcColor, destColor);
    } else if (blendMode == 28) { // 28 corresponds to the luminosity blend mode
        blendedColor = luminosity(srcColor, destColor);
    } else {
        // default case, if no blend mode is specified, just output the source color
        blendedColor = srcColor;
    }

    return blendedColor;
}

void main()
{
   vec4 srcColor = texture(u_texture_src, texCoord);
   vec4 destColor = texture(u_texture_dest, texCoord);
   vec4 finalColor = blend(srcColor, destColor, blendMode);

   fragColor = finalColor;
}
