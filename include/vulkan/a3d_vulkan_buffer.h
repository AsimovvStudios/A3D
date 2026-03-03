#pragma once

#include <vulkan/vulkan.h>

#include "a3d.h"

typedef struct
{
	VkBuffer buff;
	VkDeviceMemory mem;
	VkDeviceSize size;
} a3d_buffer;

bool a3d_vk_create_buffer(
    a3d* e,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags props,
    a3d_buffer* out_buff,
    const void* initial_data
);
void a3d_vk_destroy_buffer(a3d* e, a3d_buffer* buff);
