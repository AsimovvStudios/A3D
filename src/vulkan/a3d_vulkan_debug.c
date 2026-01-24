#include <vulkan/vulkan.h>

#include "vulkan/a3d_vulkan_debug.h"
#define A3D_LOG_TAG "VK"
#include "a3d_logging.h"

#if A3D_VK_VALIDATION

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* message,
	void* user_data
);
bool a3d_vk_debug_init(a3d *e);
bool a3d_vk_debug_shutdown(a3d *e);

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* message,
	void* user_data
)
{
	(void)type;
	(void)user_data;

	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		A3D_LOG_ERROR("%s", message->pMessage);
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		A3D_LOG_WARN("%s", message->pMessage);
	else
		A3D_LOG_DEBUG("%s", message->pMessage);
	return VK_FALSE;
}

bool a3d_vk_debug_init(a3d *e)
{
	PFN_vkCreateDebugUtilsMessengerEXT pfn_create =
		(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			e->vk.instance, "vkCreateDebugUtilsMessengerEXT"
	);
	if (!pfn_create) {
		A3D_LOG_ERROR("vkCreateDebugUtilsMessengerEXT not found (validation_layers missing?)");
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_callback,
		.pUserData = e,
	};

	VkResult result = pfn_create(e->vk.instance, &debug_utils_messenger_info, NULL, &e->vk.debug_messenger);
	if (result != VK_SUCCESS) {
		A3D_LOG_WARN("failed to create debug messenger");
		return false;
	}

	A3D_LOG_INFO("created debug messenger");
	return true;
}

bool a3d_vk_debug_shutdown(a3d *e)
{
	PFN_vkDestroyDebugUtilsMessengerEXT pfn_destroy =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			e->vk.instance, "vkDestroyDebugUtilsMessengerEXT"
	);
	if (pfn_destroy && e->vk.debug_messenger) {
		pfn_destroy(e->vk.instance, e->vk.debug_messenger, NULL);
		e->vk.debug_messenger = VK_NULL_HANDLE;
		A3D_LOG_INFO("vulkan debug messenger destroyed");
	}

	return true;
}
#endif
