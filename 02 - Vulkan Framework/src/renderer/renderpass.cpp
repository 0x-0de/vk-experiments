#include "renderpass.h"

#include "vksetup.h"

#include <iostream>

render_pass::render_pass(VkSurfaceFormatKHR swapchain_format)
{
	VkAttachmentDescription attachment_color{};

	attachment_color.format = swapchain_format.format;
	attachment_color.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference attachment_color_reference{};

	attachment_color_reference.attachment = 0;
	attachment_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass_color{};

	subpass_color.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_color.colorAttachmentCount = 1;
	subpass_color.pColorAttachments = &attachment_color_reference;
	
	VkSubpassDependency subpass_dependency{};

	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo info_render_pass{};

	info_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info_render_pass.attachmentCount = 1;
	info_render_pass.pAttachments = &attachment_color;
	info_render_pass.subpassCount = 1;
	info_render_pass.pSubpasses = &subpass_color;
	info_render_pass.dependencyCount = 1;
	info_render_pass.pDependencies = &subpass_dependency;

	VkResult r = vkCreateRenderPass(get_device(), &info_render_pass, nullptr, &vk_render_pass);
	VERIFY_NORETURN(r, "Failed to create Vulkan render pass.")
	usable = VERIFY_ASSIGN(r);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable) std::cout << "[VK|INF] Created Vulkan render pass." << std::endl;
#endif
}

render_pass::~render_pass()
{
	vkDestroyRenderPass(get_device(), vk_render_pass, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan render pass." << std::endl;
#endif
}

VkRenderPass render_pass::get_handle() const
{
	return vk_render_pass;
}

bool render_pass::is_usable() const
{
	return usable;
}