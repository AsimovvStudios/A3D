#pragma once

#include <stdbool.h>

#include <SDL3/SDL.h>

typedef SDL_Scancode a3d_key;

#define A3D_KEY_INVALID SDL_SCANCODE_UNKNOWN
#define A3D_KEY(name) SDL_SCANCODE_##name

typedef struct a3d_input
{
	bool keys_down[SDL_SCANCODE_COUNT];
	bool keys_pressed[SDL_SCANCODE_COUNT];
	bool keys_released[SDL_SCANCODE_COUNT];
	float mouse_dx;
	float mouse_dy;
	bool mouse_locked;
} a3d_input;

void a3d_input_init(a3d_input* in);
void a3d_input_begin_frame(a3d_input* in);
void a3d_input_on_event(a3d_input* in, const SDL_Event* e);
bool a3d_key_down(const a3d_input* in, a3d_key key);
bool a3d_key_pressed(const a3d_input* in, a3d_key key);
bool a3d_key_released(const a3d_input* in, a3d_key key);
