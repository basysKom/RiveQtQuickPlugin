// Copyright (C) 2023 by Jeremias Bosch
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 330 core

in vec2 v_texCoord;

out vec4 fragColor;

uniform sampler2D u_texture;
uniform float u_opacity;

void main()
{
    vec4 textureColor = texture(u_texture, v_texCoord);
    fragColor = vec4(textureColor.rgb, textureColor.a * u_opacity);
}
