// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    int flipped;
    float left;
    float right;
    float top;
    float bottom;
    int useTextureNumber;
};

layout(binding = 1) uniform sampler2D u_textureA;
layout(binding = 2) uniform sampler2D u_textureB;
layout(binding = 3) uniform sampler2D u_texturePP;

vec4 drawTexture(sampler2D s_texture, vec2 texCoord) {
    if (texCoord.x >= left && texCoord.x <= right &&
        texCoord.y >= top && texCoord.y <= bottom) {
        return texture(s_texture, texCoord);
    } else {
        return vec4(0.0, 0.0, 0.0, 0.0);  // Return a transparent color for pixels outside the viewport
    }
}

void main()
{
    if (useTextureNumber == 0) {
        vec4 finalColor = drawTexture(u_textureA, texCoord);
        fragColor = finalColor * qt_Opacity;
    }
    if (useTextureNumber == 1) {
        vec4 finalColor = drawTexture(u_textureB, texCoord);
        fragColor = finalColor * qt_Opacity;
    }
    if (useTextureNumber == 2) {
        vec4 finalColor = drawTexture(u_texturePP, texCoord);
        fragColor = finalColor * qt_Opacity;
    }
}
