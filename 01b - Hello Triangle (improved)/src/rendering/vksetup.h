#ifndef _VK_SETUP_H_
#define _VK_SETUP_H_

#include "../../include/vulkan/vulkan.h"
#include "../../include/GLFW/glfw3.h"

#include <optional>
#include <vector>

//Handling all the steps necessary to create a VkDevice.
//This includes setting up the VkInstance, finding and selecting a GPU, and enabling validation layers if applicable.
namespace vksetup
{
    typedef struct
    {
        //These values are optionals to indicate if the GPU even has these queue families.
        std::optional<uint32_t> graphics_index;
        std::optional<uint32_t> presentation_index;
    } queue_family_indices;

    typedef struct
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentation_modes;
    } swap_chain_support;

    bool create_fence(VkDevice* device, VkFence* fence, bool start_signaled);
    bool create_semaphore(VkDevice* device, VkSemaphore* semaphore);

    queue_family_indices find_physical_device_queue_families();
    queue_family_indices find_physical_device_queue_families(VkPhysicalDevice device);

    bool load_shader_module(VkDevice* device, VkShaderModule* module, const char* filepath);

    swap_chain_support query_swap_chain_support();
    swap_chain_support query_swap_chain_support(VkPhysicalDevice device);

    bool setup_vulkan(VkDevice* device, GLFWwindow* window);

    bool submit_to_queue(VkQueue* queue, VkSubmitInfo* info, VkFence* fence);
    
    void destroy_vulkan();

    VkInstance get_vulkan_instance();
    VkPhysicalDevice get_selected_physical_device();
    VkSurfaceKHR get_window_surface();
}

#endif