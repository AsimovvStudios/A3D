#include <math.h>

#include "a3d_flycam.h"

#define PI (3.14159265358979323846f)

static float a3d_clampf(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

void a3d_flycam_update(a3d_camera* cam, const a3d_input* in, float dt)
{
	if (!cam || !in || dt <= 0.0f)
		return;

	const float move_speed = 10.5f;
	const float look_sensitivity = 0.0025f;
	const float pitch_limit = (85.0f * PI) / 180.0f;

	float forward = 0.0f;
	float right = 0.0f;
	float up = 0.0f;

	if (a3d_key_down(in, SDL_SCANCODE_W))
		forward += 1.0f;
	if (a3d_key_down(in, SDL_SCANCODE_S))
		forward -= 1.0f;
	if (a3d_key_down(in, SDL_SCANCODE_D))
		right += 1.0f;
	if (a3d_key_down(in, SDL_SCANCODE_A))
		right -= 1.0f;
	if (a3d_key_down(in, SDL_SCANCODE_SPACE))
		up += 1.0f;
	if (a3d_key_down(in, SDL_SCANCODE_LCTRL) || a3d_key_down(in, SDL_SCANCODE_RCTRL))
		up -= 1.0f;

	/* normalise movement vector */
	const float move_len = sqrtf(forward * forward + right * right + up * up);
	if (move_len > 0.0f) {
		const float step = (move_speed * dt) / move_len;
		a3d_camera_move_local(cam, forward * step, right * step, up * step);
	}

	/* apply mouse look if locked */
	if (in->mouse_locked) {
		cam->yaw += in->mouse_dx * look_sensitivity;
		cam->pitch = a3d_clampf(
			cam->pitch - (in->mouse_dy * look_sensitivity),
			-pitch_limit,
			pitch_limit
		);
	}

	a3d_camera_rebuild_view(cam);
}
