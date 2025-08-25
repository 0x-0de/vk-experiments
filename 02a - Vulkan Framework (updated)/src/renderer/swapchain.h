#ifndef _SWAPCHAIN_H_
#define _SWAPCHAIN_H_

#include <vector>

#include "../../lib/vulkan/vulkan.hpp"

#include "../../lib/GLFW/glfw3.h"

#include "cmdbuffer.h"
#include "renderpass.h"

#include "../utils/vksync.h"

struct swapchain_properties
{
	VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentation_modes;
};

swapchain_properties query_swapchain_properties(VkPhysicalDevice device);

class swapchain
{
	public:
		swapchain(GLFWwindow* window);
		~swapchain();
		
		bool create_framebuffers(render_pass* rp);
		bool create_framebuffers(render_pass* rp, VkImageView depth_buffer);
		
		uint32_t get_current_image_index() const;
		VkViewport get_default_viewport() const;
		VkExtent2D get_extent() const;
		VkSurfaceFormatKHR get_format() const;
		VkFramebuffer get_framebuffer(size_t index);
		VkRect2D get_full_scissor() const;
		VkSwapchainKHR get_handle() const;
		uint32_t get_image_count() const;
		VkPresentModeKHR get_presentation_mode() const;
		VkRect2D get_scissor() const;
		fence* get_sync_fence_frame_finished(uint32_t frame_index);
		semaphore* get_sync_semaphore_render_finished(uint32_t frame_index);
		VkViewport get_viewport() const;
		
		bool is_usable() const;
		
		void image_present(VkQueue queue);
		bool image_render(VkQueue queue, command_buffer* buffer);
		
		void retrieve_next_image(uint32_t* frame_index, bool* should_retry);
	private:
		GLFWwindow* window;
		
		bool create_swap_chain();
		void destroy_swap_chain();
		
		void refresh_swap_chain();
		
		bool init_image_views();
		bool init_sync_objects();
		
		VkSwapchainKHR vk_swapchain;
		
		VkExtent2D extent;
		VkPresentModeKHR presentation_mode;
		VkSurfaceFormatKHR format;

		VkViewport viewport;
		VkRect2D scissor;
		
		uint32_t image_count;
		bool usable;
		uint32_t current_image;
		
		std::vector<VkImage> images;
		std::vector<VkImageView> image_views;
		std::vector<VkFramebuffer> framebuffers;
		
		std::vector<fence*> frame_finished;
		std::vector<semaphore*> render_finished;

		fence* image_retrieved;
		render_pass* refresh_rp;
};

#endif