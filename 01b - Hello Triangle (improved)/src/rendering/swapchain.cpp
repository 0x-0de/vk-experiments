#include "swapchain.h"

#include <algorithm>
#include <iostream>
#include <limits>

//Chooses a supported format for the swap chain to use.
VkSurfaceFormatKHR choose_swap_chain_format(vksetup::swap_chain_support supported)
{
    for(auto format : supported.formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return supported.formats[0];
}

//Chooses a supported presentation mode for the swap chain to use.
VkPresentModeKHR choose_swap_chain_present_mode(vksetup::swap_chain_support supported)
{
    for(auto present_mode : supported.presentation_modes)
    {
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//Chooses a supported and/or desired resolution (extent) for the swap chain images.
VkExtent2D choose_swap_chain_extent(vksetup::swap_chain_support supported, GLFWwindow* window)
{
    auto& cap = supported.capabilities;
    if(cap.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return cap.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D extent = {(uint32_t) width, (uint32_t) height};
        extent.width = std::clamp(extent.width, cap.minImageExtent.width, cap.maxImageExtent.width);
        extent.height = std::clamp(extent.height, cap.minImageExtent.height, cap.maxImageExtent.height);

        return extent;
    }
}

Swapchain::Swapchain(VkDevice* device, GLFWwindow* window) : device(device), window(window)
{
    usable = init_swap_chain();
}

Swapchain::~Swapchain()
{
    for(uint32_t i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(*device, framebuffers[i], nullptr);
    }

    for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
    {
        vkDestroyImageView(*device, swapchain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(*device, swapchain, nullptr);
}

int Swapchain::acquire_next_image(VkSemaphore* signal, uint32_t* image_index)
{
    VkResult result = vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, *signal, VK_NULL_HANDLE, image_index);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    if((unsigned) width != swapchain_extent.width || (unsigned) height != swapchain_extent.height)
    {
        std::cout << "[INF] Window resized! Recreating the swap chain..." << std::endl;
        recreate();

        return 0;
    }
    else if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        std::cout << "[INF] Swap chain is out-of-date! Recreating..." << std::endl;
        recreate();

        return 0;
    }
    else if(result == VK_SUBOPTIMAL_KHR)
    {
        std::cout << "[INF] Swap chain is suboptimal! Recreating..." << std::endl;
        recreate();

        return 0;
    }
    else if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to present to the swap chain!" << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        
        return -1;
    }

    return 1;
}

bool Swapchain::add_render_pass(VkRenderPass render_pass)
{
    if(!init_framebuffers(render_pass)) return false;
    render_passes.push_back(render_pass);

    return true;
}

bool Swapchain::get_swapchain_viewport_and_scissor(VkViewport* viewport, VkRect2D* viewport_scissor)
{
    if(!usable) return false;

    VkViewport vp{};
    vp.x = 0;
    vp.y = 0;
    vp.width = (float) swapchain_extent.width;
    vp.height = (float) swapchain_extent.height;
    vp.minDepth = 0;
    vp.maxDepth = 1;

    VkRect2D s{};
    s.offset = {0, 0};
    s.extent = swapchain_extent;

    if(viewport != nullptr) *viewport = vp;
    if(viewport_scissor != nullptr) *viewport_scissor = s;

    return true;
}

bool Swapchain::init_framebuffers(VkRenderPass render_pass)
{
    if(!usable) return false;
    framebuffers.resize(swapchain_image_views.size());

    for(uint32_t i = 0; i < framebuffers.size(); i++)
    {
        VkImageView attachments[] = {swapchain_image_views[i]};

        VkFramebufferCreateInfo info_framebuffer{};
        info_framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info_framebuffer.renderPass = render_pass;
        info_framebuffer.attachmentCount = 1;
        info_framebuffer.pAttachments = attachments;
        info_framebuffer.width = swapchain_extent.width;
        info_framebuffer.height = swapchain_extent.height;
        info_framebuffer.layers = 1;

        VkResult result = vkCreateFramebuffer(*device, &info_framebuffer, nullptr, &framebuffers[i]);
        if(result != VK_SUCCESS)
        {
            std::cerr << "[ERR] Failed to create framebuffer." << std::endl;
            std::cerr << "[ERR] Error code: " << result << std::endl;
            return false;
        }
    }

    return true;
}

//Creating the swap chain for Vulkan to use to present images to the window.
bool Swapchain::init_swap_chain()
{
    vksetup::swap_chain_support supported_features = vksetup::query_swap_chain_support();

    VkSurfaceFormatKHR sc_format = choose_swap_chain_format(supported_features);
    VkPresentModeKHR sc_present_mode = choose_swap_chain_present_mode(supported_features);
    VkExtent2D sc_extent = choose_swap_chain_extent(supported_features, window);

    uint32_t sc_max_image_count = supported_features.capabilities.maxImageCount;

    uint32_t sc_image_count = supported_features.capabilities.minImageCount + 1;
    if(sc_image_count > sc_max_image_count && sc_max_image_count != 0)
        sc_image_count = sc_max_image_count;
    
    VkSwapchainCreateInfoKHR info_swapchain{};
    info_swapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info_swapchain.surface = vksetup::get_window_surface();
    info_swapchain.presentMode = sc_present_mode;
    info_swapchain.minImageCount = sc_image_count;
    info_swapchain.imageFormat = sc_format.format;
    info_swapchain.imageColorSpace = sc_format.colorSpace;
    info_swapchain.imageExtent = sc_extent;
    info_swapchain.imageArrayLayers = 1;
    info_swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    vksetup::queue_family_indices indices = vksetup::find_physical_device_queue_families();
    uint32_t qfi[] = {indices.graphics_index.value(), indices.presentation_index.value()};

    if(indices.graphics_index == indices.presentation_index)
        info_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    else
    {
        info_swapchain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info_swapchain.queueFamilyIndexCount = 2;
        info_swapchain.pQueueFamilyIndices = qfi;
    }

    info_swapchain.preTransform = supported_features.capabilities.currentTransform;
    info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info_swapchain.clipped = VK_TRUE;

    info_swapchain.oldSwapchain = VK_NULL_HANDLE; //We'll come back to this.

    VkResult result = vkCreateSwapchainKHR(*device, &info_swapchain, nullptr, &swapchain);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan swap chain." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    swapchain_format = sc_format;
    swapchain_extent = sc_extent;

    vkGetSwapchainImagesKHR(*device, swapchain, &sc_image_count, nullptr);
    swapchain_images.resize(sc_image_count);
    swapchain_image_views.resize(sc_image_count);
    vkGetSwapchainImagesKHR(*device, swapchain, &sc_image_count, swapchain_images.data());

    for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
    {
        VkImageViewCreateInfo info_image_view{};
        info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info_image_view.image = swapchain_images[i];
        info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info_image_view.format = swapchain_format.format;
        info_image_view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info_image_view.subresourceRange.baseMipLevel = 0;
        info_image_view.subresourceRange.levelCount = 1;
        info_image_view.subresourceRange.baseArrayLayer = 0;
        info_image_view.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(*device, &info_image_view, nullptr, &swapchain_image_views[i]);
        if(result != VK_SUCCESS)
        {
            std::cerr << "[ERR] Failed to create Vulkan image view." << std::endl;
            std::cerr << "[ERR] Error code: " << result << std::endl;
            return false;
        }
    }

    return true;
}

VkSwapchainKHR Swapchain::get_swapchain() const
{
    return swapchain;
}

VkSurfaceFormatKHR Swapchain::get_swapchain_format() const
{
    return swapchain_format;
}

VkExtent2D Swapchain::get_swapchain_extent() const
{
    return swapchain_extent;
}

std::vector<VkImage> Swapchain::get_swapchain_images() const
{
    return swapchain_images;
}

std::vector<VkImageView> Swapchain::get_swapchain_image_views() const
{
    return swapchain_image_views;
}

std::vector<VkFramebuffer> Swapchain::get_swapchain_framebuffers() const
{
    return framebuffers;
}

bool Swapchain::is_usable() const
{
    return usable;
}

void Swapchain::recreate()
{
    vkDeviceWaitIdle(*device);

    for(uint32_t i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(*device, framebuffers[i], nullptr);
    }

    for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
    {
        vkDestroyImageView(*device, swapchain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(*device, swapchain, nullptr);
    
    usable = init_swap_chain();
    for(size_t i = 0; i < render_passes.size(); i++)
    {
        init_framebuffers(render_passes[i]);
    }
}