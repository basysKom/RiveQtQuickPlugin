/**
 * Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
 * Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
 * Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
 * Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
 * Copyright (C) 2018 Damien Seguin
 * Copyright (C) 2023 https://www.basyskom.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. As clarification, there
 * is no requirement that the copyright notice and permission be included in
 * binary distributions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * Luma Edge Detection
 *
 * IMPORTANT NOTICE: luma edge detection requires gamma-corrected colors, and
 * thus 'colorTex' should be a non-sRGB texture.
 */

#version 440

#define SMAA_PRESET_ULTRA

#if defined(SMAA_PRESET_HIGH)
#define SMAA_THRESHOLD 0.1
#elif defined(SMAA_PRESET_ULTRA)
#define SMAA_THRESHOLD 0.05
#endif

#ifndef SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0
#endif

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 vOffset[3];

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    int flip;
    vec2 resolution;
} ubuf;

layout(binding = 1) uniform sampler2D colorTex;

void main() {
  vec2 threshold = vec2(SMAA_THRESHOLD);

  // Calculate lumas:
  vec3 weights = vec3(0.2126, 0.7152, 0.0722);
  float L = dot(texture(colorTex, v_texcoord).rgb, weights);

  float Lleft = dot(texture(colorTex, vOffset[0].xy).rgb, weights);
  float Ltop  = dot(texture(colorTex, vOffset[0].zw).rgb, weights);

  // We do the usual threshold:
  vec4 delta;
  delta.xy = abs(L - vec2(Lleft, Ltop));
  vec2 edges = step(threshold, delta.xy);

  // Then discard if there is no edge:
  if (dot(edges, vec2(1.0, 1.0)) == 0.0)
      discard;

  // Calculate right and bottom deltas:
  float Lright = dot(texture(colorTex, vOffset[1].xy).rgb, weights);
  float Lbottom  = dot(texture(colorTex, vOffset[1].zw).rgb, weights);
  delta.zw = abs(L - vec2(Lright, Lbottom));

  // Calculate the maximum delta in the direct neighborhood:
  vec2 maxDelta = max(delta.xy, delta.zw);

  // Calculate left-left and top-top deltas:
  float Lleftleft = dot(texture(colorTex, vOffset[2].xy).rgb, weights);
  float Ltoptop = dot(texture(colorTex, vOffset[2].zw).rgb, weights);
  delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

  // Calculate the final maximum delta:
  maxDelta = max(maxDelta.xy, delta.zw);
  float finalDelta = max(maxDelta.x, maxDelta.y);

  // Local contrast adaptation:
  edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

  fragColor = vec4(edges, 0.0, 1.0);
}