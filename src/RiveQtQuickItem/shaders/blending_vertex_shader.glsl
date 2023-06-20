// Copyright (C) 2023 by Jeremias Bosch
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

uniform mat4 u_projection;
uniform bool u_applyTranform;

void main()
{
    v_texCoord = a_texCoord;
    if (u_applyTranform) {
        gl_Position =  u_projection * vec4(a_position, 0.0, 1.0);
    } else {
        gl_Position =  vec4(a_position, 0.0, 1.0);
    }

}
