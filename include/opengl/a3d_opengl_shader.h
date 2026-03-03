#pragma once

#include <stdbool.h>

unsigned int a3d_gl_compile_shader(const char* source, unsigned int type);
unsigned int a3d_gl_create_program_from_files(const char* vert_path, const char* frag_path);
unsigned int a3d_gl_link_program(unsigned int vertex_shader, unsigned int fragment_shader);
char* a3d_gl_load_shader_source(const char* filepath);
