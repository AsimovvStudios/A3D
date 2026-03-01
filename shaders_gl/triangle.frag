#version 330 core

in vec2 v_uv;

out vec4 out_colour;

uniform sampler2D u_albedo;
uniform vec4 u_tint;

void main()
{
	out_colour = texture(u_albedo, v_uv) * u_tint;
}

