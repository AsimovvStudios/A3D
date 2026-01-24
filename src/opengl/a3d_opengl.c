#include <stddef.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_gfx.h"
#define A3D_LOG_TAG "GL"
#include "a3d_logging.h"
#include "a3d_mesh.h"
#include "a3d_renderer.h"
#include "a3d_transform.h"
#include "opengl/a3d_opengl.h"
#include "opengl/a3d_opengl_shader.h"

void a3d_gl_destroy_mesh(a3d* e, a3d_mesh* mesh)
{
	(void)e;
	A3D_LOG_INFO("destroying mesh: vao=%u vbo=%u ebo=%u",
		mesh->gpu.gl.vao, mesh->gpu.gl.vbo, mesh->gpu.gl.ebo);

	if (mesh->gpu.gl.ebo) {
		glDeleteBuffers(1, &mesh->gpu.gl.ebo);
		mesh->gpu.gl.ebo = 0;
	}

	if (mesh->gpu.gl.vbo) {
		glDeleteBuffers(1, &mesh->gpu.gl.vbo);
		mesh->gpu.gl.vbo = 0;
	}

	if (mesh->gpu.gl.vao) {
		glDeleteVertexArrays(1, &mesh->gpu.gl.vao);
		mesh->gpu.gl.vao = 0;
	}

	mesh->vertex_count = 0;
	mesh->index_count = 0;

	A3D_LOG_INFO("mesh destroyed");
}

bool a3d_gl_draw_frame(a3d* e)
{
	/* clear buffers */
	glClearColor(
		e->gl.clear_colour[0],
		e->gl.clear_colour[1],
		e->gl.clear_colour[2],
		e->gl.clear_colour[3]
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* use shader program */
	glUseProgram(e->gl.program);

	/* get draw items from renderer */
	const a3d_draw_item* items = NULL;
	Uint32 item_count = 0;
	a3d_renderer_get_draw_items(e->renderer, &items, &item_count);

	/* draw each item */
	for (Uint32 i = 0; i < item_count; i++) {
		const a3d_mesh* mesh = items[i].mesh;
		const a3d_mvp* mvp = &items[i].mvp;

		if (!mesh)
			continue;

		/* compose MVP matrix */
		mat4 mvp_mat;
		a3d_mvp_compose(mvp_mat, mvp);

		/* upload MVP to shader */
		if (e->gl.u_mvp_location >= 0) {
			glUniformMatrix4fv(e->gl.u_mvp_location, 1, GL_FALSE, (const GLfloat*)mvp_mat);
		}

		/* bind VAO and draw */
		glBindVertexArray(mesh->gpu.gl.vao);
		glDrawElements(GL_TRIANGLES, (GLsizei)mesh->index_count, GL_UNSIGNED_SHORT, NULL);
	}

	/* unbind VAO */
	glBindVertexArray(0);

	/* swap buffers */
	SDL_GL_SwapWindow(e->window);

	return true;
}

bool a3d_gl_init(a3d* e)
{
	A3D_LOG_INFO("initialising OpenGL backend");


	/* create OpenGL context */
	SDL_GLContext glctx = SDL_GL_CreateContext(e->window);
	if (!glctx) {
		A3D_LOG_ERROR("failed to create OpenGL context: %s", SDL_GetError());
		return false;
	}
	e->gl.context = glctx;

	/* make context current */
	if (!SDL_GL_MakeCurrent(e->window, glctx)) {
		A3D_LOG_ERROR("failed to make OpenGL context current: %s", SDL_GetError());
		SDL_GL_DestroyContext(glctx);
		e->gl.context = NULL;
		return false;
	}

	/* vsync */
	SDL_GL_SetSwapInterval(1);

	/* load OpenGL functions */
	if (!gladLoaderLoadGL()) {
		A3D_LOG_ERROR("failed to load OpenGL functions via glad");
		SDL_GL_DestroyContext(glctx);
		e->gl.context = NULL;
		return false;
	}

	/* check GL version */
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	A3D_LOG_INFO("OpenGL version: %d.%d", major, minor);

	if (major < 3 || (major == 3 && minor < 3)) {
		A3D_LOG_ERROR("OpenGL 3.3 or higher required, got %d.%d", major, minor);
		SDL_GL_DestroyContext(glctx);
		e->gl.context = NULL;
		return false;
	}

	/* set up OpenGL state */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	/* set initial clear colour */
	e->gl.clear_colour[0] = 0.0f;
	e->gl.clear_colour[1] = 0.0f;
	e->gl.clear_colour[2] = 0.4f;
	e->gl.clear_colour[3] = 1.0f;

	/* create shader program */
	e->gl.program = a3d_gl_create_program_from_files(
		"shaders_gl/triangle.vert",
		"shaders_gl/triangle.frag"
	);
	if (e->gl.program == 0) {
		A3D_LOG_ERROR("failed to create shader program");
		SDL_GL_DestroyContext(glctx);
		e->gl.context = NULL;
		return false;
	}

	/* get uniform location */
	e->gl.u_mvp_location = glGetUniformLocation(e->gl.program, "u_mvp");
	if (e->gl.u_mvp_location < 0) {
		A3D_LOG_WARN("u_mvp uniform not found in shader");
	}

	/* set initial viewport */
	int w, h;
	SDL_GetWindowSizeInPixels(e->window, &w, &h);
	glViewport(0, 0, w, h);

	A3D_LOG_INFO("OpenGL backend initialised");
	return true;
}

bool a3d_gl_pre_window(a3d* e, SDL_WindowFlags* flags)
{
	(void)e;
	if (!flags)
		return false;

	/* request OpenGL context; log if attributes can't be set */
	bool r = 0;

	if (!(r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(MAJOR_VERSION) failed: %s", SDL_GetError());
	if (!(r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(MINOR_VERSION) failed: %s", SDL_GetError());
	if (!(r = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(CONTEXT_PROFILE) failed: %s", SDL_GetError());
	if (!(r = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(DOUBLEBUFFER) failed: %s", SDL_GetError());
	if (!(r = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(DEPTH_SIZE) failed: %s", SDL_GetError());
	if (!(r = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8)))
		A3D_LOG_WARN("SDL_GL_SetAttribute(STENCIL_SIZE) failed: %s", SDL_GetError());

	*flags |= SDL_WINDOW_OPENGL;
	return true;
}

bool a3d_gl_init_triangle(a3d* e, a3d_mesh* mesh)
{
	(void)e;
	A3D_LOG_INFO("creating triangle mesh (OpenGL)");

	/* TODO */
	a3d_vertex vertices[] = {
		{{ 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	};

	Uint16 indices[] = {0, 1, 2};

	mesh->vertex_count = 3;
	mesh->index_count = 3;
	mesh->topology = A3D_TOPO_TRIANGLES;

	/* create VAO */
	glGenVertexArrays(1, &mesh->gpu.gl.vao);
	glBindVertexArray(mesh->gpu.gl.vao);

	/* create VBO */
	glGenBuffers(1, &mesh->gpu.gl.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.gl.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* create EBO */
	glGenBuffers(1, &mesh->gpu.gl.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpu.gl.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	/* set up vertex attributes */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(a3d_vertex),
		(void*)offsetof(a3d_vertex, position)
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(a3d_vertex),
		(void*)offsetof(a3d_vertex, colour)
	);

	/* unbind VAO */
	glBindVertexArray(0);

	A3D_LOG_INFO("created triangle mesh: vao=%u vbo=%u ebo=%u",
		mesh->gpu.gl.vao, mesh->gpu.gl.vbo, mesh->gpu.gl.ebo);
	return true;
}

bool a3d_gl_resize(a3d* e)
{
	int w, h;
	SDL_GetWindowSizeInPixels(e->window, &w, &h);

	if (w == 0 || h == 0) {
		A3D_LOG_WARN("window minimised, skipping resize");
		return false;
	}

	glViewport(0, 0, w, h);
	A3D_LOG_INFO("OpenGL viewport resized to %dx%d", w, h);
	return true;
}

void a3d_gl_set_clear_colour(a3d* e, float r, float g, float b, float a)
{
	e->gl.clear_colour[0] = r;
	e->gl.clear_colour[1] = g;
	e->gl.clear_colour[2] = b;
	e->gl.clear_colour[3] = a;
}

void a3d_gl_shutdown(a3d* e)
{
	A3D_LOG_INFO("shutting down OpenGL backend");

	if (e->gl.program) {
		glDeleteProgram(e->gl.program);
		e->gl.program = 0;
	}

	if (e->gl.context) {
		SDL_GL_DestroyContext((SDL_GLContext)e->gl.context);
		e->gl.context = NULL;
	}

	A3D_LOG_INFO("OpenGL backend shut down");
}

void a3d_gl_wait_idle(a3d* e)
{
	(void)e;
	glFinish();
}

const a3d_gfx_vtbl a3d_gl_vtbl = {
    .pre_window = a3d_gl_pre_window,
	.init = a3d_gl_init,
	.shutdown = a3d_gl_shutdown,
	.draw_frame = a3d_gl_draw_frame,
	.recreate_or_resize = a3d_gl_resize,
	.set_clear_colour = a3d_gl_set_clear_colour,
	.wait_idle = a3d_gl_wait_idle,
	.mesh_init_triangle = a3d_gl_init_triangle,
	.mesh_destroy = a3d_gl_destroy_mesh
};

