project(
  'a3d',
  'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3'
  ]
)

backend = get_option('render_backend')
cc = meson.get_compiler('c')

if get_option('buildtype') != 'release'
  add_project_arguments('-DDEBUG', language: 'c')
endif

sdl3_dep = dependency('sdl3', required: true)
cglm_dep = dependency('cglm', required: true)
m_dep = cc.find_library('m', required: true)

inc = include_directories('include', 'external')

src_engine_common = files(
  'src/a3d.c',
  'src/a3d_assets.c',
  'src/a3d_camera.c',
  'src/a3d_event.c',
  'src/a3d_flycam.c',
  'src/a3d_input.c',
  'src/a3d_map.c',
  'src/a3d_material.c',
  'src/a3d_mesh.c',
  'src/a3d_mesh_obj.c',
  'src/a3d_renderer.c',
  'src/a3d_texture.c',
  'src/a3d_transform.c',
  'src/a3d_window.c'
)

src_tests = files('tests/main.c')

if backend == 'gl'
  add_project_arguments('-DBACKEND_GL', language: 'c')

  gl_dep = dependency('gl', required: true)
  backend_deps = [gl_dep]
  src_backend = files(
    'src/opengl/a3d_opengl.c',
    'src/opengl/a3d_opengl_shader.c',
    'src/opengl/a3d_opengl_texture.c',
    'external/glad/gl.c'
  )
else
  add_project_arguments('-DBACKEND_VK', language: 'c')

  vulkan_dep = dependency('vulkan', required: true)
  backend_deps = [vulkan_dep]
  src_backend = files(
    'src/vulkan/a3d_vulkan.c',
    'src/vulkan/a3d_vulkan_buffer.c',
    'src/vulkan/a3d_vulkan_debug.c',
    'src/vulkan/a3d_vulkan_pipeline.c'
  )
endif

engine = static_library(
  'a3d',
  sources: src_engine_common + src_backend,
  include_directories: inc,
  dependencies: [sdl3_dep, cglm_dep, m_dep] + backend_deps
)

a3d_dep = declare_dependency(
  include_directories: inc,
  link_with: engine,
  dependencies: [sdl3_dep, cglm_dep, m_dep] + backend_deps
)

executable(
  'a3d_test',
  sources: src_tests,
  include_directories: inc,
  dependencies: [a3d_dep]
)

