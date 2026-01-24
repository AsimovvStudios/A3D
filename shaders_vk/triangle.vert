#version 450

layout(push_constant) uniform Push {
	mat4 mvp;
} pc;

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 out_color;

void main()
{
	vec4 pos = vec4(in_pos, 0.0, 1.0);
	gl_Position = pc.mvp * pos;
	out_color = in_color;
}
