#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_gpu.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "a3d.h"
#define A3D_LOG_TAG "ASSET"
#include "a3d_logging.h"
#include "a3d_assets.h"

static const char* A3D_BUILTIN_CHECKER_KEY = "__a3d_fallback_path__";

static Uint32 a3d_assets_find_free_texture_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_mesh_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_material_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_shader_slot(const a3d_assets* assets);
static void a3d_assets_copy_str(char* dst, size_t dst_size, const char* src);
static bool a3d_assets_normalize_path(const char* path, char out[A3D_ASSET_PATH_MAX]);
static bool a3d_assets_material_equal(const a3d_material* a, const a3d_material* b);

bool a3d_assets_init(a3d_assets* assets)
{
	if (!assets)
		return false;

	memset(assets, 0, sizeof(*assets));
	return true;
}

void a3d_assets_shutdown(a3d* e, a3d_assets* assets)
{
	if (!assets || !e)
		return;

	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++)
	{
		if (!assets->meshes[i].used)
			continue;
		a3d_destroy_mesh(e, &assets->meshes[i].value);
	}

	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++)
	{
		if (!assets->textures[i].used)
			continue;
		a3d_texture_destroy(e, &assets->textures[i].value);
	}

	memset(assets, 0, sizeof(*assets));
}

a3d_texture_handle a3d_assets_load_texture(a3d* e, a3d_assets* assets, const char* path, bool srgb)
{
	if (!e || !assets)
	{
		A3D_LOG_ERROR("a3d_assets_load_texture: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* use checkerboard texture if no path given */
	char normalized[A3D_ASSET_PATH_MAX];
	if (!path || path[0] == '\0')
	{
		a3d_assets_copy_str(normalized, sizeof(normalized), A3D_BUILTIN_CHECKER_KEY);
		srgb = false;
	}
	else if (!a3d_assets_normalize_path(path, normalized))
	{
		A3D_LOG_ERROR("failed to normalize texture path '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if texture already loaded */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++)
	{
		a3d_asset_texture_slot* slot = &assets->textures[i];
		if (!slot->used)
			continue;
		if (slot->srgb != srgb)
			continue;
		if (strcmp(slot->path, normalized) != 0)
			continue;
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_texture_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE)
	{
		A3D_LOG_ERROR("texture pool full; cannot load '%s'", path ? path : A3D_BUILTIN_CHECKER_KEY);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* load texture into slot */
	a3d_asset_texture_slot* tex = &assets->textures[handle - 1];
	if (!a3d_texture_load(e, &tex->value, path, srgb))
	{
		A3D_LOG_ERROR("failed to load texture '%s'", path ? path : A3D_BUILTIN_CHECKER_KEY);
		return A3D_ASSET_INVALID_HANDLE;
	}

	tex->used = true;
	tex->refs = 1;
	tex->srgb = srgb;
	a3d_assets_copy_str(tex->path, sizeof(tex->path), normalized);
	return handle;
}

a3d_mesh_handle a3d_assets_load_obj_mesh(a3d* e, a3d_assets* assets, const char* path)
{
	if (!e || !assets || !path || path[0] == '\0')
	{
		A3D_LOG_ERROR("a3d_assets_load_obj_mesh: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	char normalized[A3D_ASSET_PATH_MAX];
	if (!a3d_assets_normalize_path(path, normalized))
	{
		A3D_LOG_ERROR("failed to normalize mesh path '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if mesh already loaded */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++)
	{
		a3d_asset_mesh_slot* slot = &assets->meshes[i];
		if (!slot->used)
			continue;
		if (strcmp(slot->path, normalized) != 0)
			continue;
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_mesh_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE)
	{
		A3D_LOG_ERROR("mesh pool full; cannot load '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* load mesh into slot */
	a3d_asset_mesh_slot* mesh = &assets->meshes[handle - 1];
	if (!a3d_mesh_load_obj(e, &mesh->value, path))
	{
		A3D_LOG_ERROR("failed to load OBJ '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	mesh->used = true;
	mesh->refs = 1;
	a3d_assets_copy_str(mesh->path, sizeof(mesh->path), normalized);
	return handle;
}

a3d_shader_handle a3d_assets_register_shader(a3d_assets* assets, const char* name, Uint32 shader_id)
{
	if (!assets || shader_id == 0)
	{
		A3D_LOG_ERROR("a3d_assets_register_shader: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if shader already registered (by name) */
	if (name && name[0] != '\0')
	{
		for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++)
		{
			a3d_asset_shader_slot* slot = &assets->shaders[i];
			if (!slot->used)
				continue;
			if (strcmp(slot->name, name) != 0)
				continue;
			if (slot->shader_id != shader_id)
			{
				A3D_LOG_ERROR("shader name '%s' already registered with a different id", name);
				return A3D_ASSET_INVALID_HANDLE;
			}
			slot->refs++;
			return i + 1;
		}
	}

	/* check if shader already registered (by id) */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++)
	{
		a3d_asset_shader_slot* slot = &assets->shaders[i];
		if (!slot->used)
			continue;
		if (slot->shader_id != shader_id)
			continue;
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_shader_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE)
	{
		A3D_LOG_ERROR("shader pool full; cannot register '%s'", name ? name : "unnamed");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* register shader into slot */
	a3d_asset_shader_slot* shader = &assets->shaders[handle - 1];
	shader->used = true;
	shader->refs = 1;
	shader->shader_id = shader_id;
	a3d_assets_copy_str(shader->name, sizeof(shader->name), name ? name : "unnamed");
	return handle;
}

a3d_material_handle a3d_assets_create_material(a3d_assets* assets, const a3d_material* material)
{
	if (!assets)
	{
		A3D_LOG_ERROR("a3d_assets_create_material: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	a3d_material m;
	if (material)
		m = *material;
	else
		a3d_material_init(&m);

	if (m.shader != A3D_ASSET_INVALID_HANDLE &&
	    (m.shader > A3D_ASSETS_MAX_SHADERS || !assets->shaders[m.shader - 1].used))
	{
		A3D_LOG_ERROR("a3d_assets_create_material: invalid shader handle=%u", m.shader);
		return A3D_ASSET_INVALID_HANDLE;
	}
	if (m.albedo != A3D_ASSET_INVALID_HANDLE &&
	    (m.albedo > A3D_ASSETS_MAX_TEXTURES || !assets->textures[m.albedo - 1].used))
	{
		A3D_LOG_ERROR("a3d_assets_create_material: invalid albedo handle=%u", m.albedo);
		return A3D_ASSET_INVALID_HANDLE;
	}

	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MATERIALS; i++)
	{
		a3d_asset_material_slot* slot = &assets->materials[i];
		if (!slot->used)
			continue;
		if (!a3d_assets_material_equal(&slot->value, &m))
			continue;
		slot->refs++;
		return i + 1;
	}

	/* check if material already exists */
	Uint32 handle = a3d_assets_find_free_material_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE)
	{
		A3D_LOG_ERROR("material pool full");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* create material in slot */
	a3d_asset_material_slot* out = &assets->materials[handle - 1];
	out->used = true;
	out->refs = 1;
	out->value = m;

	if (m.shader != A3D_ASSET_INVALID_HANDLE)
		assets->shaders[m.shader - 1].refs++;
	if (m.albedo != A3D_ASSET_INVALID_HANDLE)
		assets->textures[m.albedo - 1].refs++;

	return handle;
}

a3d_material_handle a3d_assets_create_textured_material(
    a3d_assets* assets, a3d_shader_handle shader, a3d_texture_handle texture, const float tint[4]
)
{
	a3d_material m;
	a3d_material_init(&m);
	m.shader = shader;
	m.albedo = texture;
	if (tint)
	{
		m.tint[0] = tint[0];
		m.tint[1] = tint[1];
		m.tint[2] = tint[2];
		m.tint[3] = tint[3];
	}

	return a3d_assets_create_material(assets, &m);
}

void a3d_assets_release_texture(a3d* e, a3d_assets* assets, a3d_texture_handle handle)
{
	if (!e || !assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_TEXTURES)
		return;

	/* check if slot is valid and in use */
	a3d_asset_texture_slot* slot = &assets->textures[handle - 1];
	if (!slot->used || slot->refs == 0)
		return;

	if (slot->refs > 1)
	{
		slot->refs--;
		return;
	}

	a3d_texture_destroy(e, &slot->value);
	memset(slot, 0, sizeof(*slot));
}

void a3d_assets_release_mesh(a3d* e, a3d_assets* assets, a3d_mesh_handle handle)
{
	if (!e || !assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_MESHES)
		return;

	/* check if slot is valid and in use */
	a3d_asset_mesh_slot* slot = &assets->meshes[handle - 1];
	if (!slot->used || slot->refs == 0)
		return;

	if (slot->refs > 1)
	{
		slot->refs--;
		return;
	}

	a3d_destroy_mesh(e, &slot->value);
	memset(slot, 0, sizeof(*slot));
}

void a3d_assets_release_material(a3d* e, a3d_assets* assets, a3d_material_handle handle)
{
	if (!e || !assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_MATERIALS)
		return;

	/* check if slot is valid and in use */
	a3d_asset_material_slot* slot = &assets->materials[handle - 1];
	if (!slot->used || slot->refs == 0)
		return;

	if (slot->refs > 1)
	{
		slot->refs--;
		return;
	}

	a3d_shader_handle shader_dep = slot->value.shader;
	a3d_texture_handle texture_dep = slot->value.albedo;
	memset(slot, 0, sizeof(*slot));

	if (texture_dep != A3D_ASSET_INVALID_HANDLE)
		a3d_assets_release_texture(e, assets, texture_dep);
	if (shader_dep != A3D_ASSET_INVALID_HANDLE)
		a3d_assets_release_shader(assets, shader_dep);
}

void a3d_assets_release_shader(a3d_assets* assets, a3d_shader_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_SHADERS)
		return;

	/* check if slot is valid and in use */
	a3d_asset_shader_slot* slot = &assets->shaders[handle - 1];
	if (!slot->used || slot->refs == 0)
		return;

	if (slot->refs > 1)
	{
		slot->refs--;
		return;
	}

	memset(slot, 0, sizeof(*slot));
}

Uint32 a3d_assets_shader_id(const a3d_assets* assets, a3d_shader_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_SHADERS)
		return 0;

	/* check if slot is valid and in use */
	const a3d_asset_shader_slot* shader = &assets->shaders[handle - 1];
	if (!shader->used)
		return 0;

	return shader->shader_id;
}

const a3d_texture* a3d_assets_get_texture(const a3d_assets* assets, a3d_texture_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_TEXTURES)
		return NULL;

	/* check if slot is valid and in use */
	const a3d_asset_texture_slot* slot = &assets->textures[handle - 1];
	if (!slot->used)
		return NULL;

	return &slot->value;
}

const a3d_mesh* a3d_assets_get_mesh(const a3d_assets* assets, a3d_mesh_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_MESHES)
		return NULL;

	/* check if slot is valid and in use */
	const a3d_asset_mesh_slot* slot = &assets->meshes[handle - 1];
	if (!slot->used)
		return NULL;

	return &slot->value;
}

const a3d_material* a3d_assets_get_material(const a3d_assets* assets, a3d_material_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_MATERIALS)
		return NULL;

	/* check if slot is valid and in use */
	const a3d_asset_material_slot* slot = &assets->materials[handle - 1];
	if (!slot->used)
		return NULL;

	return &slot->value;
}

a3d_material* a3d_assets_get_material_mut(a3d_assets* assets, a3d_material_handle handle)
{
	if (!assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_MATERIALS)
		return NULL;

	/* check if slot is valid and in use */
	a3d_asset_material_slot* slot = &assets->materials[handle - 1];
	if (!slot->used)
		return NULL;

	return &slot->value;
}

static Uint32 a3d_assets_find_free_texture_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++)
	{
		if (!assets->textures[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_mesh_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++)
	{
		if (!assets->meshes[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_material_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MATERIALS; i++)
	{
		if (!assets->materials[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_shader_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++)
	{
		if (!assets->shaders[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static void a3d_assets_copy_str(char* dst, size_t dst_size, const char* src)
{
	if (!dst || dst_size == 0)
		return;

	if (!src)
	{
		dst[0] = '\0';
		return;
	}

	snprintf(dst, dst_size, "%s", src);
}

static bool a3d_assets_normalize_path(const char* path, char out[A3D_ASSET_PATH_MAX])
{
	if (!path || !out)
		return false;

	/* if calling has "./game" or "game" */
	while (path[0] == '.' && path[1] == '/')
		path += 2;

	a3d_assets_copy_str(out, A3D_ASSET_PATH_MAX, path);

	return true;
}

static bool a3d_assets_material_equal(const a3d_material* a, const a3d_material* b)
{
	if (!a || !b)
		return false;

	if (a->shader != b->shader || a->albedo != b->albedo)
		return false;

	return (
	    a->tint[0] == b->tint[0] && a->tint[1] == b->tint[1] && a->tint[2] == b->tint[2] && a->tint[3] == b->tint[3]
	);
}
