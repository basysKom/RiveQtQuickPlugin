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

#version 440

#define SMAA_PRESET_ULTRA

#if defined(SMAA_PRESET_HIGH)
#define SMAA_MAX_SEARCH_STEPS 16
#elif defined(SMAA_PRESET_ULTRA)
#define SMAA_MAX_SEARCH_STEPS 32
#endif

#define mad(a, b, c) (a * b + c)

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec2 vPixCoord;
layout(location = 2) out vec4 vOffset[3];

layout(std140, binding = 0) uniform buf {
    vec2 resolution;
    int flip;
} ubuf;

void main() {
  vec4 SMAA_RT_METRICS = vec4(1.0 / ubuf.resolution.x, 1.0 / ubuf.resolution.y, ubuf.resolution.x, ubuf.resolution.y);

  v_texcoord = vec2(texcoord.x, texcoord.y);
  if (ubuf.flip != 0)
    v_texcoord.y = 1.0 - v_texcoord.y;

  vPixCoord = v_texcoord * SMAA_RT_METRICS.zw;
  
  // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
  vOffset[0] = mad(SMAA_RT_METRICS.xyxy, vec4(-0.25, -0.125,  1.25, -0.125), v_texcoord.xyxy);
  vOffset[1] = mad(SMAA_RT_METRICS.xyxy, vec4(-0.125, -0.25, -0.125,  1.25), v_texcoord.xyxy);

  // And these for the searches, they indicate the ends of the loops:
  vOffset[2] = mad(
    SMAA_RT_METRICS.xxyy,
    vec4(-2.0, 2.0, -2.0, 2.0) * float(SMAA_MAX_SEARCH_STEPS),
    vec4(vOffset[0].xz, vOffset[1].yw)
  );

  gl_Position = position;
}