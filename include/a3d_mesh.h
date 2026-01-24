#pragma once

#include <SDL3/SDL_stdinc.h>

typedef struct a3d a3d;

typedef enum {
	A3D_TOPO_TRIANGLES
} a3d_topology;

typedef struct a3d_vertex {
	float       position[2];
	float       colour[3];
} a3d_vertex;

struct a3d_mesh {
	Uint32      vertex_count;
	Uint32      index_count;
	a3d_topology topology;

	union {
		/* Vulkan handles */
		struct {
			void*       vertex_buffer_buff;
			void*       vertex_buffer_mem;
			Uint64      vertex_buffer_size;
			void*       index_buffer_buff;
			void*       index_buffer_mem;
			Uint64      index_buffer_size;
		} vk;

		/* OpenGL handles */
		struct {
			unsigned int vao;
			unsigned int vbo;
			unsigned int ebo;
		} gl;
	} gpu;
};

typedef struct a3d_mesh a3d_mesh;

void a3d_destroy_mesh(a3d* e, a3d_mesh* mesh);
bool a3d_init_triangle(a3d* e, a3d_mesh* mesh);
