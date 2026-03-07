#include <string.h>

#include "a3d_input.h"

static bool a3d_input_valid_scancode(SDL_Scancode sc);

static bool a3d_input_valid_scancode(SDL_Scancode sc)
{
	return (Uint32)sc < (Uint32)SDL_SCANCODE_COUNT;
}

void a3d_input_init(a3d_input* in)
{
	if (!in)
		return;

	memset(in, 0, sizeof(*in));
}

void a3d_input_begin_frame(a3d_input* in)
{
	if (!in)
		return;

	memset(in->keys_pressed, 0, sizeof(in->keys_pressed));
	memset(in->keys_released, 0, sizeof(in->keys_released));
	in->mouse_dx = 0.0f;
	in->mouse_dy = 0.0f;
}

void a3d_input_on_event(a3d_input* in, const SDL_Event* e)
{
	if (!in || !e)
		return;

	switch (e->type)
	{
		case SDL_EVENT_KEY_DOWN:
		{
			SDL_Scancode sc = e->key.scancode;
			if (!a3d_input_valid_scancode(sc))
				break;

			if (!in->keys_down[sc])
				in->keys_pressed[sc] = true;
			in->keys_down[sc] = true;
			break;
		}
		case SDL_EVENT_KEY_UP:
		{
			SDL_Scancode sc = e->key.scancode;
			if (!a3d_input_valid_scancode(sc))
				break;

			if (in->keys_down[sc])
				in->keys_released[sc] = true;
			in->keys_down[sc] = false;
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
			in->mouse_dx += e->motion.xrel;
			in->mouse_dy += e->motion.yrel;
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			memset(in->keys_down, 0, sizeof(in->keys_down));
			break;
		default:
			break;
	}
}

bool a3d_key_down(const a3d_input* in, a3d_key key)
{
	SDL_Scancode sc = key;
	if (!in || !a3d_input_valid_scancode(sc))
		return false;
	return in->keys_down[sc];
}

bool a3d_key_pressed(const a3d_input* in, a3d_key key)
{
	SDL_Scancode sc = key;
	if (!in || !a3d_input_valid_scancode(sc))
		return false;
	return in->keys_pressed[sc];
}

bool a3d_key_released(const a3d_input* in, a3d_key key)
{
	SDL_Scancode sc = key;
	if (!in || !a3d_input_valid_scancode(sc))
		return false;
	return in->keys_released[sc];
}
