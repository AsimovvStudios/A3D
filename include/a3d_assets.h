#pragma once

#include <stdbool.h>
#include <SDL3/SDL_stdinc.h>

#include "a3d_material.h"
#include "a3d_mesh.h"
#include "a3d_texture.h"

typedef struct a3d a3d;

typedef Uint32 a3d_texture_handle;
typedef Uint32 a3d_mesh_handle;
typedef Uint32 a3d_material_handle;
typedef Uint32 a3d_shader_handle;

#define A3D_ASSET_INVALID_HANDLE 0u

#define A3D_ASSET_PATH_MAX 512
#define A3D_ASSET_NAME_MAX 64

#define A3D_ASSETS_MAX_TEXTURES 256
#define A3D_ASSETS_MAX_MESHES 256
#define A3D_ASSETS_MAX_MATERIALS 256
#define A3D_ASSETS_MAX_SHADERS 64

typedef struct a3d_asset_texture_slot {
	bool        used;
	bool        srgb;
	Uint32      refs;
	char        path[A3D_ASSET_PATH_MAX];
	a3d_texture value;
} a3d_asset_texture_slot;

typedef struct a3d_asset_mesh_slot {
	bool        used;
	Uint32      refs;
	char        path[A3D_ASSET_PATH_MAX];
	a3d_mesh    value;
} a3d_asset_mesh_slot;

typedef struct a3d_asset_material_slot {
	bool        used;
	Uint32      refs;
	a3d_shader_handle shader;
	a3d_texture_handle albedo_texture;
	a3d_material value;
} a3d_asset_material_slot;

typedef struct a3d_asset_shader_slot {
	bool        used;
	Uint32      refs;
	char        name[A3D_ASSET_NAME_MAX];
	Uint32      shader_id;
} a3d_asset_shader_slot;

typedef struct a3d_assets {
	a3d_asset_texture_slot textures[A3D_ASSETS_MAX_TEXTURES];
	a3d_asset_mesh_slot meshes[A3D_ASSETS_MAX_MESHES];
	a3d_asset_material_slot materials[A3D_ASSETS_MAX_MATERIALS];
	a3d_asset_shader_slot shaders[A3D_ASSETS_MAX_SHADERS];
} a3d_assets;

bool a3d_assets_init(a3d_assets* assets);
void a3d_assets_shutdown(a3d* e, a3d_assets* assets);

a3d_texture_handle a3d_assets_load_texture(a3d* e, a3d_assets* assets, const char* path, bool srgb);
a3d_mesh_handle a3d_assets_load_obj_mesh(a3d* e, a3d_assets* assets, const char* path);
a3d_shader_handle a3d_assets_register_shader(a3d_assets* assets, const char* name, Uint32 shader_id);
a3d_material_handle a3d_assets_create_material(a3d_assets* assets, const a3d_material* material);
a3d_material_handle a3d_assets_create_textured_material(
	a3d_assets* assets,
	a3d_shader_handle shader,
	a3d_texture_handle texture,
	const float tint[4]
);
void a3d_assets_release_texture(a3d* e, a3d_assets* assets, a3d_texture_handle handle);
void a3d_assets_release_mesh(a3d* e, a3d_assets* assets, a3d_mesh_handle handle);
void a3d_assets_release_material(a3d* e, a3d_assets* assets, a3d_material_handle handle);
void a3d_assets_release_shader(a3d_assets* assets, a3d_shader_handle handle);

Uint32 a3d_assets_shader_id(const a3d_assets* assets, a3d_shader_handle handle);

const a3d_texture* a3d_assets_get_texture(const a3d_assets* assets, a3d_texture_handle handle);
const a3d_mesh* a3d_assets_get_mesh(const a3d_assets* assets, a3d_mesh_handle handle);
const a3d_material* a3d_assets_get_material(const a3d_assets* assets, a3d_material_handle handle);
a3d_material* a3d_assets_get_material_mut(a3d_assets* assets, a3d_material_handle handle);

