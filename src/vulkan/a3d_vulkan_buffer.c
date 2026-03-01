#include <string.h>
#include <vulkan/vulkan.h>

#include "a3d.h"
#define A3D_LOG_TAG "VK"
#include "a3d_logging.h"
#include "vulkan/a3d_vulkan_buffer.h"

static Uint32 find_memory_type(a3d* e, Uint32 type_filter, VkMemoryPropertyFlags properties);

bool a3d_vk_create_buffer(
	a3d* e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
	a3d_buffer* out_buff, const void* initial_data
)
{
	A3D_LOG_INFO("creating buffer of %lu bytes", (Uint64)size);

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkResult result = vkCreateBuffer(e->vk.logical, &buffer_info, NULL, &out_buff->buff);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateBuffer failed with code %d", result);
		return false;
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(e->vk.logical, out_buff->buff, &mem_requirements);

	/* check type index before passing */
	Uint32 type_index = find_memory_type(e, mem_requirements.memoryTypeBits, props);
	if (type_index == UINT32_MAX) {
		A3D_LOG_ERROR("no suitable memory type for buffer");
		vkDestroyBuffer(e->vk.logical, out_buff->buff, NULL);
		out_buff->buff = VK_NULL_HANDLE;
		return false;
	}
	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_requirements.size,
		.memoryTypeIndex = type_index
	};
	
	result = vkAllocateMemory(e->vk.logical, &allocate_info, NULL, &out_buff->mem);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkAllocateMemory failed with code %d", result);
		vkDestroyBuffer(e->vk.logical, out_buff->buff, NULL);
		out_buff->buff = VK_NULL_HANDLE;
		return false;
	}

	/* upload initial data */
	if (initial_data) {
		void* data = NULL;
		result = vkMapMemory(e->vk.logical, out_buff->mem, 0, size, 0, &data);
		if (result != VK_SUCCESS) {
			A3D_LOG_ERROR("vkMapMemory failed with code %d", result);
			vkFreeMemory(e->vk.logical, out_buff->mem, NULL);
			out_buff->mem = VK_NULL_HANDLE;
			vkDestroyBuffer(e->vk.logical, out_buff->buff, NULL);
			out_buff->buff = VK_NULL_HANDLE;
			return false;
		}
		memcpy(data, initial_data, size);
		vkUnmapMemory(e->vk.logical, out_buff->mem);
	}

	result = vkBindBufferMemory(e->vk.logical, out_buff->buff, out_buff->mem, 0);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkBindBufferMemory failed with code %d", result);
		vkFreeMemory(e->vk.logical, out_buff->mem, NULL);
		out_buff->mem = VK_NULL_HANDLE;
		vkDestroyBuffer(e->vk.logical, out_buff->buff, NULL);
		out_buff->buff = VK_NULL_HANDLE;
		return false;
	}

	out_buff->size = size;
	A3D_LOG_INFO("created buffer");

	return true;
}

void a3d_vk_destroy_buffer(a3d* e, a3d_buffer* buff)
{
	if (buff->buff) {
		vkDestroyBuffer(e->vk.logical, buff->buff, NULL);
		buff->buff = VK_NULL_HANDLE;
	}

	if (buff->mem) {
		vkFreeMemory(e->vk.logical, buff->mem, NULL);
		buff->mem = VK_NULL_HANDLE;
	}

	buff->size = 0;
	A3D_LOG_INFO("destroyed buffer");
}

static Uint32 find_memory_type(a3d* e, Uint32 type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(e->vk.physical, &mem_properties);

	for (Uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	A3D_LOG_ERROR("failed to find suitable memory type");
	return UINT32_MAX;
}

