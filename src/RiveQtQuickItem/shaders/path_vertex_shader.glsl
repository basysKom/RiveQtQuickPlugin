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
uniform float u_gradientRadius;
uniform vec2 u_viewportDimensions;

uniform mat4 u_transform;
uniform mat4 u_projection;

out vec2 v_texCoord;
out vec2 originalVertex;

void main() {
    v_texCoord = a_texCoord;
    originalVertex = a_position;
    gl_Position = u_projection * u_transform * vec4(a_position, 0.0, 1.0);
}

