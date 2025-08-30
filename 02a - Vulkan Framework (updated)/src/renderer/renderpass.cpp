#include "renderpass.h"

#include "vksetup.h"

#include <iostream>

render_pass_attachment render_pass::create_render_pass_attachment_default_color(VkFormat format)
{
	render_pass_attachment rpa;
	
	rpa.format = format;
	rpa.load_operation = VK_ATTACHMENT_LOAD_OP_CLEAR;
	rpa.store_operation = VK_ATTACHMENT_STORE_OP_STORE;
	rpa.start_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rpa.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	rpa.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	return rpa;
}

render_pass_attachment render_pass::create_render_pass_attachment_default_depth(VkFormat format)
{
	render_pass_attachment rpa;
	
	rpa.format = format;
	rpa.load_operation = VK_ATTACHMENT_LOAD_OP_CLEAR;
	rpa.store_operation = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	rpa.start_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rpa.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	rpa.reference_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	return rpa;
}

render_pass::render_pass()
{
	usable = false;
}

render_pass::~render_pass()
{
	vkDestroyRenderPass(get_device(), vk_render_pass, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan render pass." << std::endl;
#endif
}

void render_pass::add_attachment(render_pass_attachment rpa)
{
	attachments.push_back(rpa);
}

void render_pass::add_subpass(subpass sp)
{
	subpasses.push_back(sp);
}

bool render_pass::build()
{
	std::vector<VkAttachmentDescription> attachment_descriptions;
	std::vector<VkAttachmentReference> attachment_references;
	
	std::vector<std::vector<VkAttachmentReference> > subpass_attachment_references;
	std::vector<VkSubpassDescription> subpass_descriptions;
	
	for(size_t i = 0; i < attachments.size(); i++)
	{
		render_pass_attachment rpa = attachments[i];
		
		VkAttachmentDescription desc{};
		desc.format = rpa.format;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = rpa.load_operation;
		desc.storeOp = rpa.store_operation;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = rpa.start_layout;
		desc.finalLayout = rpa.final_layout;
		attachment_descriptions.push_back(desc);
		
		VkAttachmentReference ref{};
		ref.attachment = i;
		ref.layout = rpa.reference_layout;
		attachment_references.push_back(ref);
	}
	
	subpass_attachment_references.resize(subpasses.size());
	
	for(size_t i = 0; i < subpasses.size(); i++)
	{
		uint32_t num_attachment_indices = subpasses[i].color_attachment_indices.size();
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = subpasses[i].pipeline_bind_point;
		subpass.colorAttachmentCount = num_attachment_indices;
		subpass_attachment_references[i].resize(num_attachment_indices);
		for(size_t j = 0; j < num_attachment_indices; j++)
			subpass_attachment_references[i][j] = attachment_references[subpasses[i].color_attachment_indices[j]];
		subpass.pColorAttachments = subpass_attachment_references[i].data();
		if(subpasses[i].depth_attachment_index != UINT32_MAX)
			subpass.pDepthStencilAttachment = attachment_references.data() + subpasses[i].depth_attachment_index;
		subpass_descriptions.push_back(subpass);
	}
	
	VkSubpassDependency subpass_dependency{};

	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo info_render_pass{};
	info_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info_render_pass.attachmentCount = attachment_descriptions.size();
	info_render_pass.pAttachments = attachment_descriptions.data();
	info_render_pass.subpassCount = subpass_descriptions.size();
	info_render_pass.pSubpasses = subpass_descriptions.data();
	info_render_pass.dependencyCount = 1;
	info_render_pass.pDependencies = &subpass_dependency;
	
	VkResult r = vkCreateRenderPass(get_device(), &info_render_pass, nullptr, &vk_render_pass);
	VERIFY(r, "Failed to create Vulkan render pass.")
	usable = VERIFY_ASSIGN(r);
	
#ifdef DEBUG_PRINT_SUCCESS
	if(usable) std::cout << "[VK|INF] Created Vulkan render pass." << std::endl;
#endif
	return true;
}

VkRenderPass render_pass::get_handle() const
{
	return vk_render_pass;
}

bool render_pass::is_usable() const
{
	return usable;
}