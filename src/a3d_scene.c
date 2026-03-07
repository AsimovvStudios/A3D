#include <limits.h>
#include <string.h>

#include <cglm/cglm.h>

#include "a3d.h"
#define A3D_LOG_TAG "SCENE"
#include "a3d_logging.h"
#include "a3d_scene.h"
#include "a3d_transform.h"

#define A3D_SCENE_FREE_NONE ((Uint16)UINT16_MAX)

static Uint16 a3d_scene_next_generation(Uint16 generation);
static a3d_entity a3d_scene_make_entity(Uint16 slot, Uint16 generation);
static Uint16 a3d_scene_entity_slot(a3d_entity entity);
static Uint16 a3d_scene_entity_generation(a3d_entity entity);
static bool a3d_scene_resolve_entity(const a3d_scene* scene, a3d_entity entity, Uint16* out_slot, Uint16* out_dense);
static void a3d_scene_rebuild_model(a3d_scene* scene, Uint32 dense_index);

static Uint16 a3d_scene_next_generation(Uint16 generation)
{
	generation++;
	if (generation == 0)
		generation = 1;
	return generation;
}

static a3d_entity a3d_scene_make_entity(Uint16 slot, Uint16 generation)
{
	return ((Uint32)generation << 16) | ((Uint32)slot + 1u);
}

static Uint16 a3d_scene_entity_slot(a3d_entity entity)
{
	Uint16 slot_plus_one = (Uint16)(entity & 0xFFFFu);
	if (slot_plus_one == 0)
		return A3D_SCENE_FREE_NONE;
	return (Uint16)(slot_plus_one - 1u);
}

static Uint16 a3d_scene_entity_generation(a3d_entity entity)
{
	return (Uint16)((entity >> 16) & 0xFFFFu);
}

static bool a3d_scene_resolve_entity(const a3d_scene* scene, a3d_entity entity, Uint16* out_slot, Uint16* out_dense)
{
	if (!scene || entity == A3D_ENTITY_INVALID)
		return false;

	Uint16 slot = a3d_scene_entity_slot(entity);
	if (slot == A3D_SCENE_FREE_NONE || slot >= A3D_SCENE_MAX_ENTITIES)
		return false;

	const a3d_scene_slot* slot_info = &scene->slots[slot];
	if (!slot_info->alive)
		return false;

	if (slot_info->generation != a3d_scene_entity_generation(entity))
		return false;

	Uint16 dense = slot_info->dense_index;
	if ((Uint32)dense >= scene->live_count)
		return false;

	if (scene->dense_to_slot[dense] != slot)
		return false;

	if (out_slot)
		*out_slot = slot;
	if (out_dense)
		*out_dense = dense;
	return true;
}

static void a3d_scene_rebuild_model(a3d_scene* scene, Uint32 dense_index)
{
	mat4* model = &scene->models[dense_index];
	const float* pos = scene->positions[dense_index];
	const float* rot = scene->rotations_deg[dense_index];
	const float* scale = scene->scales[dense_index];

	glm_mat4_identity(*model);
	glm_translate(*model, (vec3){pos[0], pos[1], pos[2]});
	glm_rotate_x(*model, glm_rad(rot[0]), *model);
	glm_rotate_y(*model, glm_rad(rot[1]), *model);
	glm_rotate_z(*model, glm_rad(rot[2]), *model);
	glm_scale(*model, (vec3){scale[0], scale[1], scale[2]});

	scene->model_dirty[dense_index] = false;
}

void a3d_scene_init(a3d_scene* scene)
{
	if (!scene)
		return;

	memset(scene, 0, sizeof(*scene));
	for (Uint16 i = 0; i < A3D_SCENE_MAX_ENTITIES; i++)
	{
		scene->slots[i].generation = 1;
		scene->slots[i].dense_index = 0;
		scene->slots[i].alive = false;
		scene->slots[i].next_free = (i + 1 < A3D_SCENE_MAX_ENTITIES) ? (Uint16)(i + 1) : A3D_SCENE_FREE_NONE;
	}

	scene->free_head = 0;
	scene->live_count = 0;
}

void a3d_scene_clear(a3d_scene* scene)
{
	if (!scene)
		return;

	/* clear slots */
	for (Uint16 i = 0; i < A3D_SCENE_MAX_ENTITIES; i++)
	{
		scene->slots[i].alive = false;
		scene->slots[i].dense_index = 0;
		scene->slots[i].generation = a3d_scene_next_generation(scene->slots[i].generation);
		scene->slots[i].next_free = (i + 1 < A3D_SCENE_MAX_ENTITIES) ? (Uint16)(i + 1) : A3D_SCENE_FREE_NONE;
	}

	scene->free_head = 0;
	scene->live_count = 0;
}

a3d_entity a3d_scene_spawn(a3d_scene* scene, const a3d_scene_spawn_desc* desc)
{
	if (!scene || !desc)
		return A3D_ENTITY_INVALID;

	if (desc->mesh == A3D_ASSET_INVALID_HANDLE)
		return A3D_ENTITY_INVALID;

	if (scene->live_count >= A3D_SCENE_MAX_ENTITIES || scene->free_head == A3D_SCENE_FREE_NONE)
		return A3D_ENTITY_INVALID;

	/* allocate slot from free list */
	Uint16 slot = scene->free_head;
	a3d_scene_slot* slot_info = &scene->slots[slot];
	scene->free_head = slot_info->next_free;

	Uint16 dense = (Uint16)scene->live_count;
	scene->live_count++;

	/* populate slot and dense data */
	slot_info->alive = true;
	slot_info->dense_index = dense;
	slot_info->next_free = A3D_SCENE_FREE_NONE;
	scene->dense_to_slot[dense] = slot;

	/* copy entity data */
	scene->meshes[dense] = desc->mesh;
	scene->materials[dense] = desc->material;
	memcpy(scene->positions[dense], desc->position, sizeof(scene->positions[dense]));
	memcpy(scene->rotations_deg[dense], desc->rotation_deg, sizeof(scene->rotations_deg[dense]));
	memcpy(scene->scales[dense], desc->scale, sizeof(scene->scales[dense]));
	scene->model_dirty[dense] = true;

	return a3d_scene_make_entity(slot, slot_info->generation);
}

bool a3d_scene_destroy(a3d_scene* scene, a3d_entity entity)
{
	if (!scene)
		return false;

	Uint16 slot = 0;
	Uint16 dense = 0;
	if (!a3d_scene_resolve_entity(scene, entity, &slot, &dense))
		return false;

	/* if not last dense, move last dense to this dense */
	Uint16 last_dense = (Uint16)(scene->live_count - 1u);
	if (dense != last_dense)
	{
		Uint16 moved_slot = scene->dense_to_slot[last_dense];
		scene->dense_to_slot[dense] = moved_slot;
		scene->slots[moved_slot].dense_index = dense;
		scene->meshes[dense] = scene->meshes[last_dense];
		scene->materials[dense] = scene->materials[last_dense];
		memcpy(scene->positions[dense], scene->positions[last_dense], sizeof(scene->positions[dense]));
		memcpy(scene->rotations_deg[dense], scene->rotations_deg[last_dense], sizeof(scene->rotations_deg[dense]));
		memcpy(scene->scales[dense], scene->scales[last_dense], sizeof(scene->scales[dense]));
		memcpy(scene->models[dense], scene->models[last_dense], sizeof(scene->models[dense]));
		scene->model_dirty[dense] = scene->model_dirty[last_dense];
	}

	scene->live_count--;

	/* add slot back to free list */
	a3d_scene_slot* slot_info = &scene->slots[slot];
	slot_info->alive = false;
	slot_info->dense_index = 0;
	slot_info->generation = a3d_scene_next_generation(slot_info->generation);
	slot_info->next_free = scene->free_head;
	scene->free_head = slot;

	return true;
}

bool a3d_scene_set_transform(
    a3d_scene* scene, a3d_entity entity, const float pos[3], const float rot_deg[3], const float scale[3]
)
{
	if (!scene || !pos || !rot_deg || !scale)
		return false;

	Uint16 dense = 0;
	if (!a3d_scene_resolve_entity(scene, entity, NULL, &dense))
		return false;

	/* update transform, mark model dirty */
	memcpy(scene->positions[dense], pos, sizeof(scene->positions[dense]));
	memcpy(scene->rotations_deg[dense], rot_deg, sizeof(scene->rotations_deg[dense]));
	memcpy(scene->scales[dense], scale, sizeof(scene->scales[dense]));
	scene->model_dirty[dense] = true;
	return true;
}

bool a3d_scene_submit(a3d* engine, a3d_scene* scene, const mat4 view, const mat4 proj)
{
	if (!engine || !scene)
		return false;

	/* submit all entities to renderer */
	bool ok = true;
	for (Uint32 i = 0; i < scene->live_count; i++)
	{
		if (scene->meshes[i] == A3D_ASSET_INVALID_HANDLE)
		{
			ok = false;
			continue;
		}

		if (scene->model_dirty[i])
			a3d_scene_rebuild_model(scene, i);

		a3d_mvp mvp;
		memcpy(mvp.model, scene->models[i], sizeof(mvp.model));
		memcpy(mvp.view, view, sizeof(mvp.view));
		memcpy(mvp.proj, proj, sizeof(mvp.proj));

		if (!a3d_submit_mesh_material_handle(engine, scene->meshes[i], scene->materials[i], &mvp))
			ok = false;
	}

	return ok;
}
