#pragma once

#include <stdbool.h>

#include <SDL3/SDL_stdinc.h>
#include <cglm/cglm.h>

#include "a3d_handles.h"

typedef struct a3d a3d;
typedef Uint32 a3d_entity;

#define A3D_ENTITY_INVALID 0u
#define A3D_SCENE_MAX_ENTITIES 4096

typedef struct a3d_scene_spawn_desc
{
	a3d_mesh_handle mesh;
	a3d_material_handle material;
	float position[3];
	float rotation_deg[3];
	float scale[3];
} a3d_scene_spawn_desc;

typedef struct a3d_scene_slot
{
	Uint16 generation;
	Uint16 dense_index;
	Uint16 next_free;
	bool alive;
} a3d_scene_slot;

typedef struct a3d_scene
{
	a3d_scene_slot slots[A3D_SCENE_MAX_ENTITIES];
	Uint16 dense_to_slot[A3D_SCENE_MAX_ENTITIES];
	a3d_mesh_handle meshes[A3D_SCENE_MAX_ENTITIES];
	a3d_material_handle materials[A3D_SCENE_MAX_ENTITIES];
	float positions[A3D_SCENE_MAX_ENTITIES][3];
	float rotations_deg[A3D_SCENE_MAX_ENTITIES][3];
	float scales[A3D_SCENE_MAX_ENTITIES][3];
	mat4 models[A3D_SCENE_MAX_ENTITIES];
	bool model_dirty[A3D_SCENE_MAX_ENTITIES];
	Uint16 free_head;
	Uint32 live_count;
} a3d_scene;

void a3d_scene_init(a3d_scene* scene);
void a3d_scene_clear(a3d_scene* scene);
a3d_entity a3d_scene_spawn(a3d_scene* scene, const a3d_scene_spawn_desc* desc);
bool a3d_scene_destroy(a3d_scene* scene, a3d_entity entity);
bool a3d_scene_set_transform(
    a3d_scene* scene, a3d_entity entity, const float pos[3], const float rot_deg[3], const float scale[3]
);
bool a3d_scene_submit(a3d* engine, a3d_scene* scene, const mat4 view, const mat4 proj);
