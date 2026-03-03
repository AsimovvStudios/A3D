#pragma once

#include "a3d_handles.h"

typedef struct a3d_material {
	a3d_shader_handle shader;
	a3d_texture_handle albedo;
	float       tint[4];
} a3d_material;

void a3d_material_init(a3d_material* material);
void a3d_material_set_tint(a3d_material* material, float r, float g, float b, float a);

