#pragma once

#include <cglm/cglm.h>

#include "a3d.h"

struct a3d_mvp
{
	mat4 model;
	mat4 view;
	mat4 proj;
};

void a3d_mvp_compose(mat4 out, const a3d_mvp* in);
