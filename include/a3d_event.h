#pragma once

#include <SDL3/SDL.h>

#include "a3d.h"

bool a3d_event_add_handler(a3d* e, Uint32 type, a3d_event_handler fn);
void a3d_event_pump(a3d* e);
void a3d_event_on_close_requested(a3d* e, const SDL_Event* ev);
void a3d_event_on_quit(a3d* e, const SDL_Event* ev);
void a3d_event_on_resize(a3d* e, const SDL_Event* ev);
void a3d_event_handle(a3d* e, const SDL_Event* ev);
const char* a3d_event_sdl_to_str(SDL_EventType type);
