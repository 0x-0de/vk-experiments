#ifndef _SWAPCHAIN_H_
#define _SWAPCHAIN_H_

#include "vksetup.h"

class Swapchain
{
    public:
        Swapchain(VkDevice* device, GLFWwindow* window);
        ~Swapchain();

        void acquire_next_image(VkSemaphore* signal, uint32_t* image_index);

        VkSwapchainKHR get_swapchain() const;
        VkSurfaceFormatKHR get_swapchain_format() const;
        VkExtent2D get_swapchain_extent() const;

        bool get_swapchain_viewport_and_scissor(VkViewport* viewport, VkRect2D* viewport_scissor);

        bool init_framebuffers(VkRenderPass render_pass);

        bool is_usable() const;

        std::vector<VkImage> get_swapchain_images() const;
        std::vector<VkImageView> get_swapchain_image_views() const;
        std::vector<VkFramebuffer> get_swapchain_framebuffers() const;
    private:
        VkDevice* device;
        GLFWwindow* window;

        VkSwapchainKHR swapchain;
        VkSurfaceFormatKHR swapchain_format;
        VkExtent2D swapchain_extent;

        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> framebuffers;

        bool usable;

        bool init_swap_chain();
};

#endif