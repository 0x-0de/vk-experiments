#ifndef _VKSETUP_H_
#define _VKSETUP_H_

#include <optional>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "../../lib/vulkan/vulkan.hpp"

#include "../../lib/GLFW/glfw3.h"

#define VERIFY(x,y) if(x != VK_SUCCESS) { report_vulkan_error(y, x); return false; }
#define VERIFY_NORETURN(x,y) if(x != VK_SUCCESS) { report_vulkan_error(y, x); }
#define VERIFY_ASSIGN(x) x == VK_SUCCESS

struct queue_family
{
	std::optional<uint32_t> queue_index_graphics;
	std::optional<uint32_t> queue_index_transfer;
	std::optional<uint32_t> queue_index_present;
};

void report_vulkan_error(const char* msg, VkResult error_code);

queue_family find_physical_device_queue_families(VkPhysicalDevice device);

void deinit_vulkan_application();
bool init_vulkan_application(GLFWwindow* window);

VkSurfaceKHR get_window_surface();
VkPhysicalDevice get_selected_physical_device();
VkDevice get_device();

#endif