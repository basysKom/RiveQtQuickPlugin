// Copyright (C) 2023 by Jeremias Bosch
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

uniform mat4 u_model;
uniform mat4 u_projection;
uniform mat4 u_transform;

void main()
{
    v_texCoord = a_texCoord;
    gl_Position =  u_projection * u_transform * vec4(a_position, 0.0, 1.0);
    gl_Position.y = -1 * gl_Position.y;
}
