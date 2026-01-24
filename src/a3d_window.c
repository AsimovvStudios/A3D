#include <SDL3/SDL.h>

#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_window.h"

SDL_Window* a3d_create_window(const char* title, int w, int h, SDL_WindowFlags flags)
{
	SDL_Window* window = SDL_CreateWindow(title, w, h, flags);
	if (!window)
		A3D_LOG_ERROR("failed to create SDL window");

	return window;
}
