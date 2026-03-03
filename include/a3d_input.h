#pragma once

#include <stdbool.h>

#include <SDL3/SDL.h>

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
bool a3d_key_down(const a3d_input* in, SDL_Scancode sc);
bool a3d_key_pressed(const a3d_input* in, SDL_Scancode sc);
bool a3d_key_released(const a3d_input* in, SDL_Scancode sc);
