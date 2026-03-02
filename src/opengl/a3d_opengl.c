#include <stddef.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_gfx.h"
#define A3D_LOG_TAG "GL"
#include "a3d_logging.h"
#include "a3d_material.h"
#include "a3d_mesh.h"
#include "a3d_renderer.h"
#include "a3d_texture.h"
#include "a3d_transform.h"
#include "opengl/a3d_opengl.h"
#include "opengl/a3d_opengl_shader.h"

static unsigned int a3d_gl_select_program(a3d* e, const a3d_material* material);
static void a3d_gl_get_uniform_locations(
	const a3d* e,
	unsigned int program,
	int* u_mvp,
	int* u_albedo,
	int* u_tint,
	int* u_use_instance_mvp
);

void a3d_gl_destroy_mesh(a3d* e, a3d_mesh* mesh)
{
	(void)e;
	if (!mesh)
		return;

	A3D_LOG_INFO("destroying mesh: vao=%u vbo=%u ebo=%u instance=%u",
		mesh->gpu.gl.vao, mesh->gpu.gl.vbo, mesh->gpu.gl.ebo, mesh->gpu.gl.instance_vbo);

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

	if (mesh->gpu.gl.instance_vbo) {
		glDeleteBuffers(1, &mesh->gpu.gl.instance_vbo);
		mesh->gpu.gl.instance_vbo = 0;
	}

	mesh->vertex_count = 0;
	mesh->index_count = 0;

	A3D_LOG_INFO("mesh destroyed");
}

bool a3d_gl_draw_frame(a3d* e)
{
	if (!e || !e->renderer) {
		A3D_LOG_ERROR("a3d_gl_draw_frame: invalid engine state");
		return false;
	}

	/* enforce known GL state each frame */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	/* clear buffers */
	glClearColor(
		e->gl.clear_colour[0],
		e->gl.clear_colour[1],
		e->gl.clear_colour[2],
		e->gl.clear_colour[3]
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* get draw items from renderer */
	const a3d_draw_item* items = NULL;
	Uint32 item_count = 0;
	a3d_renderer_get_draw_items(e->renderer, &items, &item_count);

	unsigned int bound_program = 0;
	unsigned int bound_texture = 0;
	int u_mvp = -1;
	int u_albedo = -1;
	int u_tint = -1;
	int u_use_instance_mvp = -1;

	/* draw each item */
	for (Uint32 i = 0; i < item_count; i++) {
		const a3d_mesh* mesh = items[i].mesh;
		const a3d_material* material = items[i].material ? items[i].material : &e->gl.default_material;
		const a3d_texture* texture = material->albedo ? material->albedo : &e->gl.missing_texture;
		unsigned int texture_id = texture->gpu.gl.id ? texture->gpu.gl.id : e->gl.missing_texture.gpu.gl.id;
		unsigned int program = a3d_gl_select_program(e, material);
		float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};

		if (!mesh)
			continue;
		if (mesh->gpu.gl.vao == 0 || mesh->index_count == 0)
			continue;

		if (bound_program != program) {
			glUseProgram(program);
			bound_program = program;
			a3d_gl_get_uniform_locations(
				e,
				program,
				&u_mvp,
				&u_albedo,
				&u_tint,
				&u_use_instance_mvp
			);
			if (u_albedo >= 0)
				glUniform1i(u_albedo, 0);
		}

		if (material) {
			tint[0] = material->tint[0];
			tint[1] = material->tint[1];
			tint[2] = material->tint[2];
			tint[3] = material->tint[3];
		}

		if (bound_texture != texture_id) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			bound_texture = texture_id;
		}

		if (u_tint >= 0)
			glUniform4fv(u_tint, 1, tint);

		glBindVertexArray(mesh->gpu.gl.vao);
		if (items[i].instance_count > 0) {
			const mat4* instance_mvps = &e->renderer->instance_mvp_pool[items[i].instance_offset];
			if (u_use_instance_mvp >= 0) {
				if (!mesh->gpu.gl.instance_vbo) {
					A3D_LOG_ERROR("mesh has no instance buffer");
					continue;
				}

				glUniform1i(u_use_instance_mvp, 1);
				glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.gl.instance_vbo);
				glBufferData(
					GL_ARRAY_BUFFER,
					(GLsizeiptr)(sizeof(mat4) * items[i].instance_count),
					instance_mvps,
					GL_STREAM_DRAW
				);
				glDrawElementsInstanced(
					GL_TRIANGLES,
					(GLsizei)mesh->index_count,
					GL_UNSIGNED_INT,
					NULL,
					(GLsizei)items[i].instance_count
				);
			}
			else {
				if (u_mvp < 0) {
					A3D_LOG_WARN("instanced draw skipped: shader has no u_mvp or u_use_instance_mvp");
					continue;
				}
				for (Uint32 j = 0; j < items[i].instance_count; j++) {
					glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*)instance_mvps[j]);
					glDrawElements(GL_TRIANGLES, (GLsizei)mesh->index_count, GL_UNSIGNED_INT, NULL);
				}
			}
		}
		else {
			mat4 mvp_mat;
			a3d_mvp_compose(mvp_mat, &items[i].mvp);
			if (u_use_instance_mvp >= 0)
				glUniform1i(u_use_instance_mvp, 0);
			if (u_mvp >= 0)
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*)mvp_mat);
			glDrawElements(GL_TRIANGLES, (GLsizei)mesh->index_count, GL_UNSIGNED_INT, NULL);
		}
	}

	/* unbind VAO */
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

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
	glEnable(GL_FRAMEBUFFER_SRGB);

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

	e->gl.u_albedo_location = glGetUniformLocation(e->gl.program, "u_albedo");
	if (e->gl.u_albedo_location < 0)
		A3D_LOG_WARN("u_albedo uniform not found in shader");

	e->gl.u_tint_location = glGetUniformLocation(e->gl.program, "u_tint");
	if (e->gl.u_tint_location < 0)
		A3D_LOG_WARN("u_tint uniform not found in shader");

	e->gl.u_use_instance_mvp_location = glGetUniformLocation(e->gl.program, "u_use_instance_mvp");
	if (e->gl.u_use_instance_mvp_location < 0)
		A3D_LOG_WARN("u_use_instance_mvp uniform not found in shader");

	if (!a3d_gl_texture_load(e, &e->gl.missing_texture, NULL, false)) {
		A3D_LOG_ERROR("failed to create missing texture fallback");
		glDeleteProgram(e->gl.program);
		e->gl.program = 0;
		SDL_GL_DestroyContext(glctx);
		e->gl.context = NULL;
		return false;
	}

	a3d_material_init(&e->gl.default_material);
	e->gl.default_material.shader = e->gl.program;
	e->gl.default_material.albedo = &e->gl.missing_texture;

	glUseProgram(e->gl.program);
	if (e->gl.u_albedo_location >= 0)
		glUniform1i(e->gl.u_albedo_location, 0);
	if (e->gl.u_use_instance_mvp_location >= 0)
		glUniform1i(e->gl.u_use_instance_mvp_location, 0);
	glUseProgram(0);

	/* set initial viewport */
	int w, h;
	SDL_GetWindowSizeInPixels(e->window, &w, &h);
	glViewport(0, 0, w, h);

	A3D_LOG_INFO("OpenGL backend initialised");
	return true;
}

bool a3d_gl_init_triangle(a3d* e, a3d_mesh* mesh)
{
	A3D_LOG_INFO("creating triangle mesh (OpenGL)");

	a3d_vertex vertices[] = {
		{{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
	};

	a3d_index indices[] = {0, 1, 2};

	return a3d_mesh_upload(e, mesh, vertices, 3, indices, 3, A3D_TOPO_TRIANGLES);
}

bool a3d_gl_mesh_upload(a3d* e, a3d_mesh* mesh,
	const a3d_vertex* vertices, Uint32 vertex_count,
	const a3d_index* indices, Uint32 index_count,
	a3d_topology topology
)
{
	if (!e || !mesh || !vertices || vertex_count == 0 || !indices || index_count == 0) {
		A3D_LOG_ERROR("a3d_gl_mesh_upload: invalid parameters");
		return false;
	}

	/* if mesh has data, destroy it first */
	if (mesh->gpu.gl.vao || mesh->gpu.gl.vbo || mesh->gpu.gl.ebo || mesh->gpu.gl.instance_vbo)
		a3d_gl_destroy_mesh(e, mesh);

	/* metadata */
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;
	mesh->topology = topology;

	/* create VAO */
	glGenVertexArrays(1, &mesh->gpu.gl.vao);
	if (!mesh->gpu.gl.vao) {
		A3D_LOG_ERROR("failed to create VAO");
		return false;
	}
	glBindVertexArray(mesh->gpu.gl.vao);

	/* create VBO */
	glGenBuffers(1, &mesh->gpu.gl.vbo);
	if (!mesh->gpu.gl.vbo) {
		A3D_LOG_ERROR("failed to create VBO");
		a3d_gl_destroy_mesh(e, mesh);
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.gl.vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(a3d_vertex) * vertex_count), vertices, GL_STATIC_DRAW);

	/* create EBO */
	glGenBuffers(1, &mesh->gpu.gl.ebo);
	if (!mesh->gpu.gl.ebo) {
		A3D_LOG_ERROR("failed to create EBO");
		a3d_gl_destroy_mesh(e, mesh);
		return false;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpu.gl.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(a3d_index) * index_count), indices, GL_STATIC_DRAW);

	/* set up vertex attributes */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		3,
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
		(void*)offsetof(a3d_vertex, normal)
	);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(a3d_vertex),
		(void*)offsetof(a3d_vertex, uv)
	);

	glGenBuffers(1, &mesh->gpu.gl.instance_vbo);
	if (!mesh->gpu.gl.instance_vbo) {
		A3D_LOG_ERROR("failed to create instance VBO");
		a3d_gl_destroy_mesh(e, mesh);
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, mesh->gpu.gl.instance_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4), NULL, GL_STREAM_DRAW);

	for (unsigned int col = 0; col < 4; col++) {
		unsigned int attrib = 3 + col;
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(
			attrib,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(mat4),
			(void*)(sizeof(float) * 4u * col)
		);
		glVertexAttribDivisor(attrib, 1);
	}

	/* unbind VAO */
	glBindVertexArray(0);

	A3D_LOG_INFO(
		"uploaded mesh (OpenGL): verts=%u indices=%u vao=%u vbo=%u ebo=%u",
		mesh->vertex_count, mesh->index_count, mesh->gpu.gl.vao, mesh->gpu.gl.vbo, mesh->gpu.gl.ebo
	);

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

	a3d_gl_texture_destroy(e, &e->gl.missing_texture);

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

static unsigned int a3d_gl_select_program(a3d* e, const a3d_material* material)
{
	unsigned int program = e->gl.program;
	if (material && material->shader != 0)
		program = material->shader;

	if (!program || !glIsProgram(program)) {
		A3D_LOG_WARN("material shader is invalid; using default shader");
		program = e->gl.program;
	}

	return program;
}

static void a3d_gl_get_uniform_locations(
	const a3d* e,
	unsigned int program,
	int* u_mvp,
	int* u_albedo,
	int* u_tint,
	int* u_use_instance_mvp
)
{
	if (!u_mvp || !u_albedo || !u_tint || !u_use_instance_mvp)
		return;

	if (program == e->gl.program) {
		*u_mvp = e->gl.u_mvp_location;
		*u_albedo = e->gl.u_albedo_location;
		*u_tint = e->gl.u_tint_location;
		*u_use_instance_mvp = e->gl.u_use_instance_mvp_location;
		return;
	}

	*u_mvp = glGetUniformLocation(program, "u_mvp");
	*u_albedo = glGetUniformLocation(program, "u_albedo");
	*u_tint = glGetUniformLocation(program, "u_tint");
	*u_use_instance_mvp = glGetUniformLocation(program, "u_use_instance_mvp");
}

const a3d_gfx_vtbl a3d_gl_vtbl = {
    .pre_window = a3d_gl_pre_window,
	.init = a3d_gl_init,
	.shutdown = a3d_gl_shutdown,
	.draw_frame = a3d_gl_draw_frame,
	.recreate_or_resize = a3d_gl_resize,
	.set_clear_colour = a3d_gl_set_clear_colour,
	.wait_idle = a3d_gl_wait_idle,
	.texture_load = a3d_gl_texture_load,
	.texture_destroy = a3d_gl_texture_destroy,
	.mesh_upload = a3d_gl_mesh_upload,
	.mesh_init_triangle = a3d_gl_init_triangle,
	.mesh_destroy = a3d_gl_destroy_mesh
};

