#version 330 core

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec3 in_color;

out vec3 v_color;

uniform mat4 u_mvp;

void main()
{
	gl_Position = u_mvp * vec4(in_pos, 0.0, 1.0);
	v_color = in_color;
}

