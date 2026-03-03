#include <string.h>

#include "a3d.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_texture.h"

bool a3d_texture_load(a3d* e, a3d_texture* texture, const char* path, bool srgb)
{
	if (!e || !texture)
	{
		A3D_LOG_ERROR("a3d_texture_load: invalid args");
		return false;
	}

	/* zero out texture so backend doesnt write to all fields */
	memset(texture, 0, sizeof(*texture));
	if (!e->gfx.v || !e->gfx.v->texture_load)
	{
		A3D_LOG_ERROR("backend does not support textures");
		return false;
	}

	return e->gfx.v->texture_load(e, texture, path, srgb);
}

void a3d_texture_destroy(a3d* e, a3d_texture* texture)
{
	if (!e || !texture)
		return;

	if (e->gfx.v && e->gfx.v->texture_destroy)
		e->gfx.v->texture_destroy(e, texture);
}
