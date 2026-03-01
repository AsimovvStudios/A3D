#include "a3d_material.h"

void a3d_material_init(a3d_material* material)
{
	if (!material)
		return;

	material->shader = 0;
	material->albedo = NULL;
	a3d_material_set_tint(material, 1.0f, 1.0f, 1.0f, 1.0f);
}

void a3d_material_set_tint(a3d_material* material, float r, float g, float b, float a)
{
	if (!material)
		return;

	material->tint[0] = r;
	material->tint[1] = g;
	material->tint[2] = b;
	material->tint[3] = a;
}

