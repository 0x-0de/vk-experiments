#ifndef _RENDERPASS_H_
#define _RENDERPASS_H_

#include "../../lib/vulkan/vulkan.hpp"

class render_pass
{
	public:
		render_pass(VkSurfaceFormatKHR swapchain_format);
		~render_pass();
				
		VkRenderPass get_handle() const;
		
		bool is_usable() const;
	private:
		VkRenderPass vk_render_pass;
		
		bool usable;
};

#endif