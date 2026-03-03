#include <stdbool.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "a3d.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_mesh.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

static const char* a3d_tinyobj_result_string(int result);
static void
a3d_tinyobj_file_reader(void* ctx, const char* filename, int is_mtl, const char* obj_filename, char** buf, size_t* len);
static bool a3d_tinyobj_read_file_alloc(const char* filename, char** out_buf, size_t* out_len);

typedef struct a3d_tinyobj_ctx
{
	char** bufs;
	size_t count;
	size_t cap;
} a3d_tinyobj_ctx;

bool a3d_mesh_load_obj(a3d* e, a3d_mesh* mesh, const char* path)
{
	if (!e || !mesh || !path)
	{
		A3D_LOG_ERROR("a3d_mesh_load_obj: invalid args");
		return false;
	}

	bool ok = false;
	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes = NULL;
	tinyobj_material_t* materials = NULL;
	size_t num_shapes = 0;
	size_t num_materials = 0;
	a3d_vertex* vertices = NULL;
	a3d_index* indices = NULL;
	a3d_tinyobj_ctx file_ctx = {0};

	/* parse OBJ file (using custom file reader) */
	tinyobj_attrib_init(&attrib);
	int result = tinyobj_parse_obj(
	    &attrib,
	    &shapes,
	    &num_shapes,
	    &materials,
	    &num_materials,
	    path,
	    a3d_tinyobj_file_reader,
	    &file_ctx,
	    TINYOBJ_FLAG_TRIANGULATE
	);
	if (result != TINYOBJ_SUCCESS)
	{
		A3D_LOG_ERROR("failed to parse OBJ '%s': %s", path, a3d_tinyobj_result_string(result));
		goto cleanup;
	}

	/* TODO */
	(void)num_shapes;
	(void)num_materials;

	if (attrib.num_vertices == 0 || attrib.num_faces == 0)
	{
		A3D_LOG_ERROR("OBJ has no renderable geometry: %s", path);
		goto cleanup;
	}

	float min_x = FLT_MAX;
	float max_x = -FLT_MAX;
	float min_z = FLT_MAX;
	float max_z = -FLT_MAX;
	for (Uint32 i = 0; i < attrib.num_vertices; i++)
	{
		float x = attrib.vertices[(size_t)i * 3 + 0];
		float z = attrib.vertices[(size_t)i * 3 + 2];
		if (x < min_x)
			min_x = x;
		if (x > max_x)
			max_x = x;
		if (z < min_z)
			min_z = z;
		if (z > max_z)
			max_z = z;
	}
	float range_x = max_x - min_x;
	float range_z = max_z - min_z;
	float inv_range_x = (range_x > 1e-8f) ? (1.0f / range_x) : 0.0f;
	float inv_range_z = (range_z > 1e-8f) ? (1.0f / range_z) : 0.0f;
	bool has_texcoords = (attrib.num_texcoords > 0);
	bool has_normals = (attrib.num_normals > 0);
	if (!has_texcoords)
		A3D_LOG_WARN("OBJ '%s' has no UVs; using generated planar UVs (XZ)", path);
	if (!has_normals)
		A3D_LOG_WARN("OBJ '%s' has no normals; using fallback +Y normals", path);

	/* if no normals or texcoords, use direct position indexing */
	bool direct_position_indexing = (!has_normals && !has_texcoords);
	Uint32 vertex_count = direct_position_indexing ? attrib.num_vertices : (attrib.num_faces * 3u);
	Uint32 index_count = attrib.num_faces * 3u;

	/* allocate vertex and index buffers */
	vertices = calloc(vertex_count, sizeof(*vertices));
	indices = malloc(sizeof(*indices) * index_count);
	if (!vertices || !indices)
	{
		A3D_LOG_ERROR("out of memory while loading OBJ '%s'", path);
		goto cleanup;
	}

	if (direct_position_indexing)
	{
		/* fill vertex buffer with positions and default normals/uvs */
		for (Uint32 i = 0; i < attrib.num_vertices; i++)
		{
			vertices[i].position[0] = attrib.vertices[(size_t)i * 3 + 0];
			vertices[i].position[1] = attrib.vertices[(size_t)i * 3 + 1];
			vertices[i].position[2] = attrib.vertices[(size_t)i * 3 + 2];
			vertices[i].normal[0] = 0.0f;
			vertices[i].normal[1] = 1.0f;
			vertices[i].normal[2] = 0.0f;
			vertices[i].uv[0] = (vertices[i].position[0] - min_x) * inv_range_x;
			vertices[i].uv[1] = (vertices[i].position[2] - min_z) * inv_range_z;
		}
	}

	size_t face_offset = 0;
	Uint32 out_index = 0;
	Uint32 out_vertex = 0;
	/* convert faces to vertex and index buffers */
	for (Uint32 face = 0; face < attrib.num_face_num_verts; face++)
	{
		int face_vertices = attrib.face_num_verts[face];
		if (face_vertices != 3)
		{
			A3D_LOG_ERROR("triangulation failed for OBJ '%s' at face %u", path, face);
			goto cleanup;
		}

		/* for each corner of face, get position/normal/uv, write to vertex/index buffers */
		for (int corner = 0; corner < 3; corner++)
		{
			tinyobj_vertex_index_t idx = attrib.faces[face_offset + (size_t)corner];
			if (idx.v_idx < 0 || (Uint32)idx.v_idx >= attrib.num_vertices)
			{
				A3D_LOG_ERROR("invalid position index in OBJ '%s' at face %u", path, face);
				goto cleanup;
			}

			/* if no normals or texcoords, use position index for vertex index */
			if (direct_position_indexing)
			{
				indices[out_index++] = (Uint32)idx.v_idx;
				continue;
			}

			/* vertex data */
			a3d_vertex* vertex = &vertices[out_vertex];
			vertex->position[0] = attrib.vertices[(size_t)idx.v_idx * 3 + 0];
			vertex->position[1] = attrib.vertices[(size_t)idx.v_idx * 3 + 1];
			vertex->position[2] = attrib.vertices[(size_t)idx.v_idx * 3 + 2];

			vertex->normal[0] = 0.0f;
			vertex->normal[1] = 1.0f;
			vertex->normal[2] = 0.0f;
			if (idx.vn_idx >= 0 && (Uint32)idx.vn_idx < attrib.num_normals)
			{
				vertex->normal[0] = attrib.normals[(size_t)idx.vn_idx * 3 + 0];
				vertex->normal[1] = attrib.normals[(size_t)idx.vn_idx * 3 + 1];
				vertex->normal[2] = attrib.normals[(size_t)idx.vn_idx * 3 + 2];
			}

			vertex->uv[0] = (vertex->position[0] - min_x) * inv_range_x;
			vertex->uv[1] = (vertex->position[2] - min_z) * inv_range_z;
			if (idx.vt_idx >= 0 && (Uint32)idx.vt_idx < attrib.num_texcoords)
			{
				vertex->uv[0] = attrib.texcoords[(size_t)idx.vt_idx * 2 + 0];
				vertex->uv[1] = attrib.texcoords[(size_t)idx.vt_idx * 2 + 1];
			}

			indices[out_index++] = out_vertex++;
		}

		face_offset += 3;
	}

	ok = a3d_mesh_upload(e, mesh, vertices, vertex_count, indices, index_count, A3D_TOPO_TRIANGLES);
	if (ok)
		A3D_LOG_INFO("loaded OBJ '%s' (verts=%u, indices=%u)", path, vertex_count, index_count);

cleanup:
	for (size_t i = 0; i < file_ctx.count; i++)
		free(file_ctx.bufs[i]);
	free(file_ctx.bufs);
	free(vertices);
	free(indices);
	tinyobj_attrib_free(&attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	return ok;
}

static const char* a3d_tinyobj_result_string(int result)
{
	switch (result)
	{
		case TINYOBJ_SUCCESS:
			return "success";
		case TINYOBJ_ERROR_EMPTY:
			return "file is empty";
		case TINYOBJ_ERROR_INVALID_PARAMETER:
			return "invalid parameter";
		case TINYOBJ_ERROR_FILE_OPERATION:
			return "file operation failed";
		default:
			return "unknown tinyobj error";
	}
}

static void
a3d_tinyobj_file_reader(void* ctx, const char* filename, int is_mtl, const char* obj_filename, char** buf, size_t* len)
{
	(void)is_mtl;
	(void)obj_filename;
	if (!buf || !len)
	{
		return;
	}

	*buf = NULL;
	*len = 0;

	char* loaded = NULL;
	size_t loaded_len = 0;
	if (!a3d_tinyobj_read_file_alloc(filename, &loaded, &loaded_len))
		return;

	a3d_tinyobj_ctx* reader_ctx = (a3d_tinyobj_ctx*)ctx;
	if (!reader_ctx)
	{
		free(loaded);
		return;
	}

	/* if needed, grow file buffer array */
	if (reader_ctx->count >= reader_ctx->cap)
	{
		size_t new_cap = (reader_ctx->cap == 0) ? 4 : (reader_ctx->cap * 2);
		char** resized = realloc(reader_ctx->bufs, sizeof(*resized) * new_cap);
		if (!resized)
		{
			free(loaded);
			return;
		}
		reader_ctx->bufs = resized;
		reader_ctx->cap = new_cap;
	}

	/* store loaded buffer in context; it can be freed later */
	reader_ctx->bufs[reader_ctx->count++] = loaded;
	*buf = loaded;
	*len = loaded_len;
}

static bool a3d_tinyobj_read_file_alloc(const char* filename, char** out_buf, size_t* out_len)
{
	if (!filename || !out_buf || !out_len)
		return false;

	*out_buf = NULL;
	*out_len = 0;

	FILE* file = fopen(filename, "rb");
	if (!file)
		return false;

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return false;
	}

	long size = ftell(file);
	if (size < 0)
	{
		fclose(file);
		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return false;
	}

	size_t len = (size_t)size;
	char* buf = malloc(len + 1);
	if (!buf)
	{
		fclose(file);
		return false;
	}

	size_t read = fread(buf, 1, len, file);
	fclose(file);
	if (read != len)
	{
		free(buf);
		return false;
	}

	buf[len] = '\0';
	*out_buf = buf;
	*out_len = len;
	return true;
}
