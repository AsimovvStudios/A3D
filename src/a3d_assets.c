#include <stdio.h>
#include <string.h>

#include "a3d.h"
#define A3D_LOG_TAG "ASSET"
#include "a3d_logging.h"
#include "a3d_assets.h"

static Uint32 a3d_assets_find_free_texture_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_mesh_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_material_slot(const a3d_assets* assets);
static Uint32 a3d_assets_find_free_shader_slot(const a3d_assets* assets);
static a3d_shader_handle a3d_assets_find_shader_handle_by_id(const a3d_assets* assets, Uint32 shader_id);
static a3d_texture_handle a3d_assets_find_texture_handle_by_ptr(const a3d_assets* assets, const a3d_texture* texture);
static void a3d_assets_copy_str(char* dst, size_t dst_size, const char* src);

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

	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++) {
		if (!assets->meshes[i].used)
			continue;
		a3d_destroy_mesh(e, &assets->meshes[i].value);
	}

	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++) {
		if (!assets->textures[i].used)
			continue;
		a3d_texture_destroy(e, &assets->textures[i].value);
	}

	memset(assets, 0, sizeof(*assets));
}

a3d_texture_handle a3d_assets_load_texture(a3d* e, a3d_assets* assets, const char* path, bool srgb)
{
	if (!e || !assets || !path || path[0] == '\0') {
		A3D_LOG_ERROR("a3d_assets_load_texture: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if texture already loaded */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++) {
		a3d_asset_texture_slot* slot = &assets->textures[i];
		if (!slot->used)
			continue;
		if (slot->srgb != srgb)
			continue;
		if (strcmp(slot->path, path) != 0)
			continue;
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_texture_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("texture pool full; cannot load '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* load texture into slot */
	a3d_asset_texture_slot* tex = &assets->textures[handle - 1];
	if (!a3d_texture_load(e, &tex->value, path, srgb)) {
		A3D_LOG_ERROR("failed to load texture '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	tex->used = true;
	tex->refs = 1;
	tex->srgb = srgb;
	a3d_assets_copy_str(tex->path, sizeof(tex->path), path);
	return handle;
}

a3d_mesh_handle a3d_assets_load_obj_mesh(a3d* e, a3d_assets* assets, const char* path)
{
	if (!e || !assets || !path || path[0] == '\0') {
		A3D_LOG_ERROR("a3d_assets_load_obj_mesh: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if mesh already loaded */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++) {
		a3d_asset_mesh_slot* slot = &assets->meshes[i];
		if (!slot->used)
			continue;
		if (strcmp(slot->path, path) != 0)
			continue;
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_mesh_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("mesh pool full; cannot load '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* load mesh into slot */
	a3d_asset_mesh_slot* mesh = &assets->meshes[handle - 1];
	if (!a3d_mesh_load_obj(e, &mesh->value, path)) {
		A3D_LOG_ERROR("failed to load OBJ '%s'", path);
		return A3D_ASSET_INVALID_HANDLE;
	}

	mesh->used = true;
	mesh->refs = 1;
	a3d_assets_copy_str(mesh->path, sizeof(mesh->path), path);
	return handle;
}

a3d_shader_handle a3d_assets_register_shader(a3d_assets* assets, const char* name, Uint32 shader_id)
{
	if (!assets || shader_id == 0) {
		A3D_LOG_ERROR("a3d_assets_register_shader: invalid args");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* check if shader already registered (by name) */
	if (name && name[0] != '\0') {
		for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++) {
			a3d_asset_shader_slot* slot = &assets->shaders[i];
			if (!slot->used)
				continue;
			if (strcmp(slot->name, name) != 0)
				continue;
			if (slot->shader_id != shader_id) {
				A3D_LOG_ERROR("shader name '%s' already registered with a different id", name);
				return A3D_ASSET_INVALID_HANDLE;
			}
			slot->refs++;
			return i + 1;
		}
	}

	/* check if shader already registered (by id) */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++) {
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
	if (handle == A3D_ASSET_INVALID_HANDLE) {
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
		return A3D_ASSET_INVALID_HANDLE;

	/* check if material already exists */
	Uint32 handle = a3d_assets_find_free_material_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("material pool full");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* create material in slot */
	a3d_asset_material_slot* out = &assets->materials[handle - 1];
	out->used = true;
	out->refs = 1;
	out->shader = A3D_ASSET_INVALID_HANDLE;
	out->albedo_texture = A3D_ASSET_INVALID_HANDLE;

	/* copy material data if provided, else init to defaults */
	if (material)
		out->value = *material;
	else
		a3d_material_init(&out->value);

	out->shader = a3d_assets_find_shader_handle_by_id(assets, out->value.shader);
	out->albedo_texture = a3d_assets_find_texture_handle_by_ptr(assets, out->value.albedo);

	if (out->shader != A3D_ASSET_INVALID_HANDLE)
		assets->shaders[out->shader - 1].refs++;
	if (out->albedo_texture != A3D_ASSET_INVALID_HANDLE)
		assets->textures[out->albedo_texture - 1].refs++;

	return handle;
}

a3d_material_handle a3d_assets_create_textured_material(
	a3d_assets* assets,
	a3d_shader_handle shader,
	a3d_texture_handle texture,
	const float tint[4]
)
{
	if (!assets)
		return A3D_ASSET_INVALID_HANDLE;

	/* validate shader and texture handles */
	if (shader != A3D_ASSET_INVALID_HANDLE &&
		(shader > A3D_ASSETS_MAX_SHADERS || !assets->shaders[shader - 1].used)) {
		A3D_LOG_ERROR("a3d_assets_create_textured_material: invalid shader handle=%u", shader);
		return A3D_ASSET_INVALID_HANDLE;
	}

	if (texture != A3D_ASSET_INVALID_HANDLE &&
		(texture > A3D_ASSETS_MAX_TEXTURES || !assets->textures[texture - 1].used)) {
		A3D_LOG_ERROR("a3d_assets_create_textured_material: invalid texture handle=%u", texture);
		return A3D_ASSET_INVALID_HANDLE;
	}

	a3d_material m;
	a3d_material_init(&m);
	m.shader = a3d_assets_shader_id(assets, shader);
	m.albedo = a3d_assets_get_texture(assets, texture);
	if (tint) {
		m.tint[0] = tint[0];
		m.tint[1] = tint[1];
		m.tint[2] = tint[2];
		m.tint[3] = tint[3];
	}

	/* check if material already exists */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MATERIALS; i++) {
		a3d_asset_material_slot* slot = &assets->materials[i];
		if (!slot->used)
			continue;
		if (slot->shader != shader)
			continue;
		if (slot->albedo_texture != texture)
			continue;
		if (slot->value.tint[0] != m.tint[0] ||
			slot->value.tint[1] != m.tint[1] ||
			slot->value.tint[2] != m.tint[2] ||
			slot->value.tint[3] != m.tint[3]) {
			continue;
		}
		slot->refs++;
		return i + 1;
	}

	/* find free slot */
	Uint32 handle = a3d_assets_find_free_material_slot(assets);
	if (handle == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("material pool full");
		return A3D_ASSET_INVALID_HANDLE;
	}

	/* create material in slot */
	a3d_asset_material_slot* out = &assets->materials[handle - 1];
	out->used = true;
	out->refs = 1;
	out->shader = shader;
	out->albedo_texture = texture;
	out->value = m;

	if (shader != A3D_ASSET_INVALID_HANDLE)
		assets->shaders[shader - 1].refs++;
	if (texture != A3D_ASSET_INVALID_HANDLE)
		assets->textures[texture - 1].refs++;

	return handle;
}

void a3d_assets_release_texture(a3d* e, a3d_assets* assets, a3d_texture_handle handle)
{
	if (!e || !assets || handle == A3D_ASSET_INVALID_HANDLE || handle > A3D_ASSETS_MAX_TEXTURES)
		return;

	/* check if slot is valid and in use */
	a3d_asset_texture_slot* slot = &assets->textures[handle - 1];
	if (!slot->used || slot->refs == 0)
		return;

	if (slot->refs > 1) {
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

	if (slot->refs > 1) {
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

	if (slot->refs > 1) {
		slot->refs--;
		return;
	}

	a3d_shader_handle shader_dep = slot->shader;
	a3d_texture_handle texture_dep = slot->albedo_texture;
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

	if (slot->refs > 1) {
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
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++) {
		if (!assets->textures[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_mesh_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MESHES; i++) {
		if (!assets->meshes[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_material_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_MATERIALS; i++) {
		if (!assets->materials[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static Uint32 a3d_assets_find_free_shader_slot(const a3d_assets* assets)
{
	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++) {
		if (!assets->shaders[i].used)
			return i + 1;
	}
	return A3D_ASSET_INVALID_HANDLE;
}

static a3d_shader_handle a3d_assets_find_shader_handle_by_id(const a3d_assets* assets, Uint32 shader_id)
{
	if (!assets || shader_id == 0)
		return A3D_ASSET_INVALID_HANDLE;

	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_SHADERS; i++) {
		if (!assets->shaders[i].used)
			continue;
		if (assets->shaders[i].shader_id == shader_id)
			return i + 1;
	}

	return A3D_ASSET_INVALID_HANDLE;
}

static a3d_texture_handle a3d_assets_find_texture_handle_by_ptr(const a3d_assets* assets, const a3d_texture* texture)
{
	if (!assets || !texture)
		return A3D_ASSET_INVALID_HANDLE;

	/* note: handle value is index+1; 0 can be reserved for invalid handle */
	for (Uint32 i = 0; i < A3D_ASSETS_MAX_TEXTURES; i++) {
		if (!assets->textures[i].used)
			continue;
		if (&assets->textures[i].value == texture)
			return i + 1;
	}

	return A3D_ASSET_INVALID_HANDLE;
}

static void a3d_assets_copy_str(char* dst, size_t dst_size, const char* src)
{
	if (!dst || dst_size == 0)
		return;

	if (!src) {
		dst[0] = '\0';
		return;
	}

	snprintf(dst, dst_size, "%s", src);
}

