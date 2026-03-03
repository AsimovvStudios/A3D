#include <string.h>

#include <glad/gl.h>

#include "a3d.h"
#define A3D_LOG_TAG "GL"
#include "a3d_logging.h"
#include "a3d_texture.h"
#include "opengl/a3d_opengl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static bool a3d_gl_upload_texture_rgba(
	a3d_texture* texture,
	const unsigned char* pixels,
	int width,
	int height,
	bool srgb,
	bool is_fallback
);
static bool a3d_gl_make_missing_texture(a3d_texture* texture);

bool a3d_gl_texture_load(a3d* e, a3d_texture* texture, const char* path, bool srgb)
{
	(void)e;
	if (!texture) {
		A3D_LOG_ERROR("a3d_gl_texture_load: texture is NULL");
		return false;
	}

	memset(texture, 0, sizeof(*texture));

	if (!path)
		return a3d_gl_make_missing_texture(texture);

	stbi_set_flip_vertically_on_load(1); /* OpenGL viewpott */
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
	if (!pixels) {
		A3D_LOG_WARN("texture load failed for '%s': %s; using missing texture", path, stbi_failure_reason());
		return a3d_gl_make_missing_texture(texture);
	}

	bool ok = a3d_gl_upload_texture_rgba(texture, pixels, width, height, srgb, false);
	stbi_image_free(pixels);
	return ok;
}

void a3d_gl_texture_destroy(a3d* e, a3d_texture* texture)
{
	(void)e;
	if (!texture)
		return;

	if (texture->gpu.gl.id) {
		glDeleteTextures(1, &texture->gpu.gl.id);
		texture->gpu.gl.id = 0;
	}

	texture->width = 0;
	texture->height = 0;
	texture->channels = 0;
	texture->srgb = false;
	texture->is_fallback = false;
}

static bool a3d_gl_upload_texture_rgba(
	a3d_texture* texture,
	const unsigned char* pixels,
	int width,
	int height,
	bool srgb,
	bool is_fallback
)
{
	if (!texture || !pixels || width <= 0 || height <= 0) {
		A3D_LOG_ERROR("a3d_gl_upload_texture_rgba: invalid args");
		return false;
	}

	unsigned int tex = 0;
	glGenTextures(1, &tex);
	if (!tex) {
		A3D_LOG_ERROR("glGenTextures failed");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* use GL_SRGB8_ALPHA8 for sRGB textures to enable auto gamma correction in shader */
	GLint internal_format = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internal_format,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		pixels
	);

	/* TODO: nearest neighbor scaling */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	texture->width = (Uint32)width;
	texture->height = (Uint32)height;
	texture->channels = 4;
	texture->srgb = srgb;
	texture->is_fallback = is_fallback;
	texture->gpu.gl.id = tex;
	return true;
}

static bool a3d_gl_make_missing_texture(a3d_texture* texture)
{
	/* 2x2 purple-black checkered pattern */
	static const unsigned char checker[16] = {
		255,  0, 255, 255,
		  0,  0,   0, 255,
		  0,  0,   0, 255,
		255,  0, 255, 255
	};
	return a3d_gl_upload_texture_rgba(texture, checker, 2, 2, false, true);
}
