#pragma once

#include <stdbool.h>

typedef struct a3d a3d;
typedef struct a3d_mesh a3d_mesh;

void a3d_gl_destroy_mesh(a3d* e, a3d_mesh* mesh);
bool a3d_gl_draw_frame(a3d* e);
bool a3d_gl_init(a3d* e);
bool a3d_gl_init_triangle(a3d* e, a3d_mesh* mesh);
bool a3d_gl_resize(a3d* e);
void a3d_gl_set_clear_colour(a3d* e, float r, float g, float b, float a);
void a3d_gl_shutdown(a3d* e);
void a3d_gl_wait_idle(a3d* e);

