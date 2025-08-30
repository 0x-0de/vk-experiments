#ifndef _RENDERPASS_H_
#define _RENDERPASS_H_

#include <cstdint>

#include "../../lib/vulkan/vulkan.hpp"

struct render_pass_attachment
{
	VkFormat format;
	VkAttachmentLoadOp load_operation;
	VkAttachmentStoreOp store_operation;
	VkImageLayout start_layout, final_layout, reference_layout;
};

struct subpass
{
	VkPipelineBindPoint pipeline_bind_point;
	std::vector<uint32_t> color_attachment_indices;
	uint32_t depth_attachment_index = UINT32_MAX;
};

class render_pass
{
	public:
		render_pass();
		~render_pass();
		
		void add_attachment(render_pass_attachment rpa);
		void add_subpass(subpass sp);
		
		bool build();
		
		static render_pass_attachment create_render_pass_attachment_default_color(VkFormat format);
		static render_pass_attachment create_render_pass_attachment_default_depth(VkFormat format);
		
		VkRenderPass get_handle() const;
		
		bool is_usable() const;
	private:
		VkRenderPass vk_render_pass;
		
		std::vector<render_pass_attachment> attachments;
		std::vector<subpass> subpasses;
		
		bool usable;
};

#endif