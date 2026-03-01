#pragma once

#include <stdbool.h>
#include <SDL3/SDL_stdinc.h>

typedef struct a3d a3d;

typedef struct a3d_texture {
	Uint32      width;
	Uint32      height;
	Uint32      channels;
	bool        srgb;
	bool        is_fallback;

	union {
		struct {
			unsigned int id;
		} gl;

		struct {
			void*       image;
			void*       memory;
			void*       view;
			void*       sampler;
		} vk;
	} gpu;
} a3d_texture;

bool a3d_texture_load(a3d* e, a3d_texture* texture, const char* path, bool srgb);
void a3d_texture_destroy(a3d* e, a3d_texture* texture);

