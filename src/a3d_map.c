#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <cglm/cglm.h>

#include "a3d.h"
#include "a3d_assets.h"
#define A3D_LOG_TAG "MAP"
#include "a3d_logging.h"
#include "a3d_map.h"
#include "a3d_transform.h"

typedef struct a3d_map_entity_build {
	char        name[A3D_MAP_ENTITY_NAME_MAX];
	char        mesh_path[A3D_ASSET_PATH_MAX];
	char        texture_path[A3D_ASSET_PATH_MAX];
	float       position[3];
	float       rotation_deg[3];
	float       scale[3];
	float       tint[4];
	bool        has_mesh;
	bool        has_pos;
	bool        has_rot;
	bool        has_scale;
} a3d_map_entity_build;

static bool a3d_map_parse_file(a3d* e, a3d_map* out, const char* path, char* err, size_t err_size);
static void a3d_map_release_entities(a3d* e, a3d_map* map);
static void a3d_map_set_error(char* err, size_t err_size, const char* fmt, ...);
static char* a3d_map_trim(char* s);
static void a3d_map_strip_inline_comment(char* s);
static bool a3d_map_parse_vec3(const char* value, float out[3]);
static bool a3d_map_parse_vec4(const char* value, float out[4]);
static bool a3d_map_parse_entity_header(const char* line, char out_name[A3D_MAP_ENTITY_NAME_MAX]);
static bool a3d_map_finalize_entity(
	a3d* e,
	a3d_map* out,
	const a3d_map_entity_build* build,
	const char* path,
	Uint32 line_no,
	char* err,
	size_t err_size
);
static bool a3d_map_get_file_mtime_ns(const char* path, Uint64* out_mtime_ns);

void a3d_map_init(a3d_map* map)
{
	if (!map)
		return;

	memset(map, 0, sizeof(*map));
	map->version = 1;
}

void a3d_map_clear(a3d* e, a3d_map* map)
{
	if (!map)
		return;

	if (!e && map->entity_count > 0) {
		A3D_LOG_WARN("a3d_map_clear called without engine; keeping loaded entities intact");
		return;
	}

	a3d_map_release_entities(e, map);
	a3d_map_init(map);
}

bool a3d_map_load(a3d* e, a3d_map* map, const char* path)
{
	if (!e || !map || !path || path[0] == '\0') {
		A3D_LOG_ERROR("a3d_map_load: invalid args");
		return false;
	}

	a3d_map parsed;
	a3d_map_init(&parsed);

	char err[512] = {0};
	if (!a3d_map_parse_file(e, &parsed, path, err, sizeof(err))) {
		A3D_LOG_ERROR("failed to load map '%s': %s", path, err[0] ? err : "unknown parse error");
		return false;
	}

	a3d_map_clear(e, map);
	*map = parsed;
	A3D_LOG_INFO("loaded map '%s' (entities=%u)", path, map->entity_count);
	return true;
}

bool a3d_map_reload_if_changed(a3d* e, a3d_map* map)
{
	if (!e || !map || map->source_path[0] == '\0')
		return false;

	/* if unable to get mtime, assume changed, reload. */
	Uint64 mtime_ns = 0;
	if (!a3d_map_get_file_mtime_ns(map->source_path, &mtime_ns))
		return false;

	if (mtime_ns == map->source_mtime_ns)
		return true;

	A3D_LOG_INFO("map file changed, reloading: %s", map->source_path);
	return a3d_map_load(e, map, map->source_path);
}

bool a3d_map_submit(a3d* e, const a3d_map* map, const mat4 view, const mat4 proj)
{
	if (!e || !map || !e->assets)
		return false;

	/* submit all entities to renderer */
	for (Uint32 i = 0; i < map->entity_count; i++) {
		const a3d_map_entity* entity = &map->entities[i];
		if (entity->mesh == A3D_ASSET_INVALID_HANDLE)
			continue;

		a3d_mvp mvp;
		glm_mat4_identity(mvp.model);
		glm_translate(mvp.model, (vec3){
			entity->position[0],
			entity->position[1],
			entity->position[2]
		});
		/* apply rotations */
		glm_rotate_x(mvp.model, glm_rad(entity->rotation_deg[0]), mvp.model);
		glm_rotate_y(mvp.model, glm_rad(entity->rotation_deg[1]), mvp.model);
		glm_rotate_z(mvp.model, glm_rad(entity->rotation_deg[2]), mvp.model);
		glm_scale(mvp.model, (vec3){
			entity->scale[0],
			entity->scale[1],
			entity->scale[2]
		});

		memcpy(mvp.view, view, sizeof(mat4));
		memcpy(mvp.proj, proj, sizeof(mat4));
		a3d_submit_mesh_material_handle(e, entity->mesh, entity->material, &mvp);
	}

	return true;
}

static bool a3d_map_parse_file(a3d* e, a3d_map* out, const char* path, char* err, size_t err_size)
{
	FILE* f = NULL;
	bool ok = false;
	a3d_map parsed;
	a3d_map_entity_build build;
	bool in_entity = false;
	char line_buf[2048];
	Uint32 line_no = 0;

	a3d_map_init(&parsed);
	if (!e || !out) {
		a3d_map_set_error(err, err_size, "parse requires engine and output map");
		return false;
	}

	/* default build state for new entities */
	memset(&build, 0, sizeof(build));
	build.scale[0] = 1.0f;
	build.scale[1] = 1.0f;
	build.scale[2] = 1.0f;
	build.tint[0] = 1.0f;
	build.tint[1] = 1.0f;
	build.tint[2] = 1.0f;
	build.tint[3] = 1.0f;

	f = fopen(path, "rb");
	if (!f) {
		a3d_map_set_error(err, err_size, "%s:%u open failed: %s", path, 0u, strerror(errno));
		goto cleanup;
	}

	snprintf(parsed.source_path, sizeof(parsed.source_path), "%s", path);
	(void)a3d_map_get_file_mtime_ns(path, &parsed.source_mtime_ns);

	/* parsing */
	while (fgets(line_buf, sizeof(line_buf), f)) {
		line_no++;
		a3d_map_strip_inline_comment(line_buf);
		char* line = a3d_map_trim(line_buf);
		if (line[0] == '\0')
			continue;

		if (!in_entity) {
			if (strncmp(line, "version", 7) == 0) {
				char* eq = strchr(line, '=');
				if (!eq) {
					a3d_map_set_error(err, err_size, "%s:%u missing '=' for version", path, line_no);
					goto cleanup;
				}
				char* value = a3d_map_trim(eq + 1);
				parsed.version = (Uint32)strtoul(value, NULL, 10);
				continue;
			}

			if (strncmp(line, "entity", 6) == 0) {
				if (parsed.entity_count >= A3D_MAP_MAX_ENTITIES) {
					a3d_map_set_error(err, err_size, "%s:%u too many entities", path, line_no);
					goto cleanup;
				}
				memset(&build, 0, sizeof(build));
				build.scale[0] = 1.0f;
				build.scale[1] = 1.0f;
				build.scale[2] = 1.0f;
				build.tint[0] = 1.0f;
				build.tint[1] = 1.0f;
				build.tint[2] = 1.0f;
				build.tint[3] = 1.0f;
				if (!a3d_map_parse_entity_header(line, build.name)) {
					a3d_map_set_error(err, err_size, "%s:%u malformed entity line", path, line_no);
					goto cleanup;
				}
				in_entity = true;
				continue;
			}

			a3d_map_set_error(err, err_size, "%s:%u unknown line outside entity: %s", path, line_no, line);
			goto cleanup;
		}

		if (strcmp(line, "end") == 0) {
			if (!a3d_map_finalize_entity(e, &parsed, &build, path, line_no, err, err_size))
				goto cleanup;
			in_entity = false;
			continue;
		}

		char* eq = strchr(line, '=');
		if (!eq) {
			a3d_map_set_error(err, err_size, "%s:%u missing '=' in entity block", path, line_no);
			goto cleanup;
		}

		*eq = '\0';
		char* key = a3d_map_trim(line);
		char* value = a3d_map_trim(eq + 1);

		/* entity properties */
		if (strcmp(key, "mesh") == 0) {
			snprintf(build.mesh_path, sizeof(build.mesh_path), "%s", value);
			build.has_mesh = (build.mesh_path[0] != '\0');
		}
		else if (strcmp(key, "name") == 0) {
			snprintf(build.name, sizeof(build.name), "%s", value);
		}
		else if (strcmp(key, "texture") == 0) {
			snprintf(build.texture_path, sizeof(build.texture_path), "%s", value);
		}
		else if (strcmp(key, "pos") == 0 || strcmp(key, "position") == 0) {
			if (!a3d_map_parse_vec3(value, build.position)) {
				a3d_map_set_error(err, err_size, "%s:%u invalid pos vec3", path, line_no);
				goto cleanup;
			}
			build.has_pos = true;
		}
		else if (strcmp(key, "rot") == 0 || strcmp(key, "rotation_deg") == 0) {
			if (!a3d_map_parse_vec3(value, build.rotation_deg)) {
				a3d_map_set_error(err, err_size, "%s:%u invalid rot vec3", path, line_no);
				goto cleanup;
			}
			build.has_rot = true;
		}
		else if (strcmp(key, "scale") == 0) {
			if (!a3d_map_parse_vec3(value, build.scale)) {
				a3d_map_set_error(err, err_size, "%s:%u invalid scale vec3", path, line_no);
				goto cleanup;
			}
			build.has_scale = true;
		}
		else if (strcmp(key, "tint") == 0) {
			if (!a3d_map_parse_vec4(value, build.tint)) {
				a3d_map_set_error(err, err_size, "%s:%u invalid tint vec4", path, line_no);
				goto cleanup;
			}
		}
		else {
			a3d_map_set_error(err, err_size, "%s:%u unknown entity key '%s'", path, line_no, key);
			goto cleanup;
		}
	}

	if (in_entity) {
		a3d_map_set_error(err, err_size, "%s:%u missing 'end' for entity", path, line_no);
		goto cleanup;
	}

	if (parsed.version != 1) {
		a3d_map_set_error(err, err_size, "%s:%u invalid version %u (expected 1)", path, 1u, parsed.version);
		goto cleanup;
	}

	*out = parsed;
	ok = true;

cleanup:
	if (f)
		fclose(f);
	if (!ok)
		a3d_map_release_entities(e, &parsed);
	return ok;
}

static void a3d_map_release_entities(a3d* e, a3d_map* map)
{
	if (!e || !map)
		return;

	a3d_assets* assets = a3d_get_assets(e);
	if (!assets)
		return;

	/* release all asset references for entities */
	for (Uint32 i = 0; i < map->entity_count; i++) {
		a3d_map_entity* entity = &map->entities[i];
		if (entity->material != A3D_ASSET_INVALID_HANDLE)
			a3d_assets_release_material(e, assets, entity->material);
		if (entity->mesh != A3D_ASSET_INVALID_HANDLE)
			a3d_assets_release_mesh(e, assets, entity->mesh);
		entity->material = A3D_ASSET_INVALID_HANDLE;
		entity->mesh = A3D_ASSET_INVALID_HANDLE;
	}
	map->entity_count = 0;
}

static void a3d_map_set_error(char* err, size_t err_size, const char* fmt, ...)
{
	if (!err || err_size == 0 || !fmt)
		return;

	va_list args;
	va_start(args, fmt);
	vsnprintf(err, err_size, fmt, args);
	va_end(args);
}

static char* a3d_map_trim(char* s)
{
	if (!s)
		return s;

	while (*s && isspace((unsigned char)*s))
		s++;

	if (*s == '\0')
		return s;

	char* end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end)) {
		*end = '\0';
		end--;
	}

	return s;
}

static void a3d_map_strip_inline_comment(char* s)
{
	if (!s)
		return;

	bool in_quotes = false;
	for (char* p = s; *p; ++p) {
		if (*p == '"')
			in_quotes = !in_quotes;
		if (!in_quotes && *p == '#') {
			*p = '\0';
			return;
		}
	}
}

static bool a3d_map_parse_vec3(const char* value, float out[3])
{
	if (!value || !out)
		return false;

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	if (sscanf(value, "%f %f %f", &x, &y, &z) != 3)
		return false;

	out[0] = x;
	out[1] = y;
	out[2] = z;
	return true;
}

static bool a3d_map_parse_vec4(const char* value, float out[4])
{
	if (!value || !out)
		return false;

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;
	if (sscanf(value, "%f %f %f %f", &x, &y, &z, &w) != 4)
		return false;

	out[0] = x;
	out[1] = y;
	out[2] = z;
	out[3] = w;
	return true;
}

static bool a3d_map_parse_entity_header(const char* line, char out_name[A3D_MAP_ENTITY_NAME_MAX])
{
	if (!line || !out_name)
		return false;

	out_name[0] = '\0';
	if (strcmp(line, "entity") == 0)
		return true;

	char name[A3D_MAP_ENTITY_NAME_MAX] = {0};
	if (sscanf(line, "entity %63s", name) == 1) {
		snprintf(out_name, A3D_MAP_ENTITY_NAME_MAX, "%s", name);
		return true;
	}

	return false;
}

static bool a3d_map_finalize_entity(
	a3d* e,
	a3d_map* out,
	const a3d_map_entity_build* build,
	const char* path,
	Uint32 line_no,
	char* err,
	size_t err_size
)
{
	if (!out || !build)
		return false;

	if (!build->has_mesh) {
		a3d_map_set_error(err, err_size, "%s:%u entity missing required 'mesh'", path, line_no);
		return false;
	}
	if (!build->has_pos) {
		a3d_map_set_error(err, err_size, "%s:%u entity missing required 'pos'", path, line_no);
		return false;
	}
	if (!build->has_rot) {
		a3d_map_set_error(err, err_size, "%s:%u entity missing required 'rot'", path, line_no);
		return false;
	}
	if (!build->has_scale) {
		a3d_map_set_error(err, err_size, "%s:%u entity missing required 'scale'", path, line_no);
		return false;
	}

	a3d_map_entity* entity = &out->entities[out->entity_count];
	memset(entity, 0, sizeof(*entity));
	if (build->name[0] != '\0')
		snprintf(entity->name, sizeof(entity->name), "%s", build->name);
	else
		snprintf(entity->name, sizeof(entity->name), "entity_%u", out->entity_count);
	entity->mesh = A3D_ASSET_INVALID_HANDLE;
	entity->material = A3D_ASSET_INVALID_HANDLE;
	entity->position[0] = build->position[0];
	entity->position[1] = build->position[1];
	entity->position[2] = build->position[2];
	entity->rotation_deg[0] = build->rotation_deg[0];
	entity->rotation_deg[1] = build->rotation_deg[1];
	entity->rotation_deg[2] = build->rotation_deg[2];
	entity->scale[0] = build->scale[0];
	entity->scale[1] = build->scale[1];
	entity->scale[2] = build->scale[2];

	a3d_assets* assets = a3d_get_assets(e);
	if (!assets) {
		a3d_map_set_error(err, err_size, "asset manager unavailable");
		return false;
	}

	entity->mesh = a3d_assets_load_obj_mesh(e, assets, build->mesh_path);
	if (entity->mesh == A3D_ASSET_INVALID_HANDLE) {
		a3d_map_set_error(err, err_size, "%s:%u failed mesh load: %s", path, line_no, build->mesh_path);
		return false;
	}

	a3d_texture_handle texture = A3D_ASSET_INVALID_HANDLE;
	if (build->texture_path[0] != '\0') {
		texture = a3d_assets_load_texture(e, assets, build->texture_path, true);
		if (texture == A3D_ASSET_INVALID_HANDLE)
			A3D_LOG_WARN("texture load failed '%s'; using missing-texture fallback", build->texture_path);
	}

	entity->material = a3d_assets_create_textured_material(
		assets,
		A3D_ASSET_INVALID_HANDLE,
		texture,
		build->tint
	);
	if (entity->material == A3D_ASSET_INVALID_HANDLE) {
		if (texture != A3D_ASSET_INVALID_HANDLE)
			a3d_assets_release_texture(e, assets, texture);
		a3d_assets_release_mesh(e, assets, entity->mesh);
		entity->mesh = A3D_ASSET_INVALID_HANDLE;
		a3d_map_set_error(err, err_size, "%s:%u failed material create for entity '%s'",
			path,
			line_no,
			entity->name
		);
		return false;
	}

	/* material keeps its own dependency reference; drop temporary load ref */
	if (texture != A3D_ASSET_INVALID_HANDLE)
		a3d_assets_release_texture(e, assets, texture);

	out->entity_count++;
	return true;
}

static bool a3d_map_get_file_mtime_ns(const char* path, Uint64* out_mtime_ns)
{
	if (!path || !out_mtime_ns)
		return false;

	struct stat st;
	if (stat(path, &st) != 0)
		return false;

#if defined(__APPLE__)
	*out_mtime_ns = (Uint64)st.st_mtimespec.tv_sec * 1000000000ull + (Uint64)st.st_mtimespec.tv_nsec;
#elif defined(__linux__)
#if defined(__USE_XOPEN2K8)
	*out_mtime_ns = (Uint64)st.st_mtim.tv_sec * 1000000000ull + (Uint64)st.st_mtim.tv_nsec;
#else
	*out_mtime_ns = (Uint64)st.st_mtime * 1000000000ull + (Uint64)st.st_mtimensec;
#endif
#elif defined(_WIN32)
	*out_mtime_ns = (Uint64)st.st_mtime * 1000000000ull;
#else
	*out_mtime_ns = (Uint64)st.st_mtime * 1000000000ull;
#endif
	return true;
}

