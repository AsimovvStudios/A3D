#include <stdio.h>
#include <stdlib.h>

#include <glad/gl.h>

#include "opengl/a3d_opengl_shader.h"
#define A3D_LOG_TAG "GL"
#include "a3d_logging.h"

unsigned int a3d_gl_compile_shader(const char* source, unsigned int type)
{
	GLuint shader = glCreateShader(type);
	if (shader == 0)
	{
		A3D_LOG_ERROR("failed to create shader");
		return 0;
	}

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLint log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			char* log = malloc((size_t)log_length);
			if (log)
			{
				glGetShaderInfoLog(shader, log_length, NULL, log);
				A3D_LOG_ERROR("shader compilation failed:\n%s", log);
				free(log);
			}
		}
		else
		{
			A3D_LOG_ERROR("shader compilation failed (no log)");
		}

		glDeleteShader(shader);
		return 0;
	}

	const char* type_str = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
	A3D_LOG_INFO("compiled %s shader", type_str);
	return shader;
}

unsigned int a3d_gl_create_program_from_files(const char* vert_path, const char* frag_path)
{
	/* load vertex shader */
	char* vert_source = a3d_gl_load_shader_source(vert_path);
	if (!vert_source)
		return 0;

	GLuint vert_shader = a3d_gl_compile_shader(vert_source, GL_VERTEX_SHADER);
	free(vert_source);
	if (vert_shader == 0)
		return 0;

	/* load fragment shader */
	char* frag_source = a3d_gl_load_shader_source(frag_path);
	if (!frag_source)
	{
		glDeleteShader(vert_shader);
		return 0;
	}

	GLuint frag_shader = a3d_gl_compile_shader(frag_source, GL_FRAGMENT_SHADER);
	free(frag_source);
	if (frag_shader == 0)
	{
		glDeleteShader(vert_shader);
		return 0;
	}

	/* link program */
	GLuint program = a3d_gl_link_program(vert_shader, frag_shader);

	/* delete shaders */
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return program;
}

unsigned int a3d_gl_link_program(unsigned int vertex_shader, unsigned int fragment_shader)
{
	GLuint program = glCreateProgram();
	if (program == 0)
	{
		A3D_LOG_ERROR("failed to create shader program");
		return 0;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLint log_length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			char* log = malloc((size_t)log_length);
			if (log)
			{
				glGetProgramInfoLog(program, log_length, NULL, log);
				A3D_LOG_ERROR("program linking failed:\n%s", log);
				free(log);
			}
		}
		else
		{
			A3D_LOG_ERROR("program linking failed (no log)");
		}
		glDeleteProgram(program);
		return 0;
	}

	/* detach shaders after linking */
	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);

	A3D_LOG_INFO("linked shader program");
	return program;
}

char* a3d_gl_load_shader_source(const char* filepath)
{
	FILE* file = fopen(filepath, "rb");
	if (!file)
	{
		A3D_LOG_ERROR("failed to open shader file: %s", filepath);
		return NULL;
	}

	/* get file size */
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size <= 0)
	{
		A3D_LOG_ERROR("shader file is empty or error: %s", filepath);
		fclose(file);
		return NULL;
	}

	/* allocate buffer */
	char* source = malloc((size_t)size + 1);
	if (!source)
	{
		A3D_LOG_ERROR("failed to allocate memory for shader source");
		fclose(file);
		return NULL;
	}

	/* read file */
	size_t read = fread(source, 1, (size_t)size, file);
	fclose(file);

	if ((long)read != size)
	{
		A3D_LOG_ERROR("failed to read shader file: %s", filepath);
		free(source);
		return NULL;
	}

	source[size] = '\0';
	A3D_LOG_INFO("loaded shader source from: %s", filepath);
	return source;
}
