#pragma once

#include <stdbool.h>

#include <SDL3/SDL_stdinc.h>
#include <cglm/cglm.h>

#include "a3d_assets.h"

typedef struct a3d a3d;

#define A3D_MAP_MAX_ENTITIES 2048
#define A3D_MAP_ENTITY_NAME_MAX 64

typedef struct a3d_map_entity
{
	char name[A3D_MAP_ENTITY_NAME_MAX];
	a3d_mesh_handle mesh;
	a3d_material_handle material;
	float position[3];
	float rotation_deg[3];
	float scale[3];
} a3d_map_entity;

typedef struct a3d_map
{
	Uint32 version;
	char source_path[A3D_ASSET_PATH_MAX];
	Uint64 source_mtime_ns;
	Uint32 entity_count;
	a3d_map_entity entities[A3D_MAP_MAX_ENTITIES];
} a3d_map;

void a3d_map_init(a3d_map* map);
void a3d_map_clear(a3d* e, a3d_map* map);
bool a3d_map_load(a3d* e, a3d_map* map, const char* path);
bool a3d_map_reload_if_changed(a3d* e, a3d_map* map);
bool a3d_map_submit(a3d* e, const a3d_map* map, const mat4 view, const mat4 proj);
