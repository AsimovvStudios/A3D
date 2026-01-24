#pragma once

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "a3d_gfx.h"

#ifndef A3D_NO_VULKAN
#define A3D_INCLUDE_VULKAN
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#endif

#if !defined(A3D_VK_VALIDATION)
#	ifndef NDEBUG
#		define A3D_VK_VALIDATION 1
#	else
#		define A3D_VK_VALIDATION 0
#	endif
#endif

/* structures */
typedef struct a3d a3d;
typedef void (*a3d_event_handler)(a3d *engine, const SDL_Event *e);
typedef struct a3d_renderer a3d_renderer;
typedef struct a3d_mesh a3d_mesh;
typedef struct a3d_mvp a3d_mvp;

#define A3D_MAX_HANDLERS 64
typedef struct {
	Uint32      type;
	a3d_event_handler fn;
} a3d_handler_slot;

struct a3d {
	/* SDL */
	SDL_Window* window;

	/* loop */
	SDL_Event   ev;
	a3d_handler_slot handlers[A3D_MAX_HANDLERS];
	Uint32 handlers_count;

	bool        running;
	bool        fb_resized;

	/* backend */
	a3d_backend backend;
	a3d_gfx     gfx;

	/* Vulkan backend */
#ifdef A3D_INCLUDE_VULKAN
	struct {
		VkInstance  instance;
		VkSurfaceKHR surface;
		VkDebugUtilsMessengerEXT debug_messenger;

		Uint32      graphics_family;
		Uint32      present_family;
		VkQueue     graphics_queue;
		VkQueue     present_queue;

		VkDevice    logical;
		VkPhysicalDevice physical;

		VkSwapchainKHR swapchain;
		VkFormat    swapchain_fmt;
		VkExtent2D  swapchain_extent;
		VkImage     swapchain_images[8];
		VkImageView swapchain_views[8];
		Uint32  swapchain_images_count;

		VkRenderPass render_pass;
		VkFramebuffer fbs[8];
		VkClearValue clear_col;

		VkCommandPool cmd_pool;
		VkCommandBuffer cmd_buffs[8];

		VkSemaphore image_available;
		VkSemaphore render_finished;
		VkFence     in_flight;

		VkPipelineLayout pipeline_layout;
		VkPipeline  pipeline;

		VkImage     depth_image;
		VkDeviceMemory depth_mem;
		VkImageView depth_view;
		VkFormat    depth_fmt;
	} vk;
#endif

	/* OpenGL backend */
	struct {
		void*       context;
		unsigned int program;
		int         u_mvp_location;
		float       clear_colour[4];
	} gl;

	a3d_renderer* renderer;
};

/* declarations */
void a3d_frame(a3d* e);
void a3d_frame_begin(a3d* e);
void a3d_frame_end(a3d* e);
bool a3d_init(a3d* e, const char* title, int w, int h);
bool a3d_init_backend(a3d* e, a3d_backend backend, const char* title, int w, int h);
void a3d_quit(a3d* e);
void a3d_set_clear_colour(a3d* e, float r, float g, float b, float a);
bool a3d_submit_mesh(a3d* e, const a3d_mesh* mesh, const a3d_mvp* mvp);
void a3d_wait_idle(a3d* e);