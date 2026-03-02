#version 330 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in mat4 in_instance_mvp;

out vec2 v_uv;

uniform mat4 u_mvp;
uniform int u_use_instance_mvp;

void main()
{
	mat4 draw_mvp = (u_use_instance_mvp != 0) ? in_instance_mvp : u_mvp;
	gl_Position = draw_mvp * vec4(in_pos, 1.0);
	v_uv = in_uv;
}
