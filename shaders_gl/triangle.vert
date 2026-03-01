#version 330 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

out vec2 v_uv;

uniform mat4 u_mvp;

void main()
{
	gl_Position = u_mvp * vec4(in_pos, 1.0);
	v_uv = in_uv;
}

