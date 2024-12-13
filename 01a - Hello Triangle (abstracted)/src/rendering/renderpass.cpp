#include "renderpass.h"

#include <iostream>

RenderPass::RenderPass(VkDevice* device, Swapchain* swapchain) : device(device), swapchain(swapchain) {}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(*device, render_pass, nullptr);
}

void RenderPass::add_attachment(VkAttachmentDescription desc)
{
    attachment_descriptions.push_back(desc);
}

void RenderPass::add_subpass(VkSubpassDescription desc, VkAttachmentReference* ref)
{
    subpass_descriptions.push_back(desc);
    attachment_references.push_back(ref);
}

void RenderPass::add_subpass_dependency(VkSubpassDependency dep)
{
    subpass_dependencies.push_back(dep);
}

void RenderPass::clear_data()
{
    for(size_t i = 0; i < attachment_references.size(); i++)
    {
        delete attachment_references[i];
    }

    attachment_descriptions.clear();
    attachment_references.clear();

    subpass_descriptions.clear();
    subpass_dependencies.clear();
}

//Creates a render pass, which is used for each framebuffer, and so also creates all of the framebuffer handles.
void RenderPass::compile_render_pass()
{
    VkRenderPassCreateInfo info_render_pass{};
    info_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info_render_pass.attachmentCount = attachment_descriptions.size();
    info_render_pass.pAttachments = attachment_descriptions.data();
    info_render_pass.subpassCount = subpass_descriptions.size();
    info_render_pass.pSubpasses = subpass_descriptions.data();
    info_render_pass.dependencyCount = subpass_dependencies.size();
    info_render_pass.pDependencies = subpass_dependencies.data();

    VkResult result = vkCreateRenderPass(*device, &info_render_pass, nullptr, &render_pass);
    clear_data();

    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan render pass." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        usable = false;
        return;
    }

    if(!swapchain->init_framebuffers(render_pass))
    {
        usable = false;
        return;
    }

    usable = true;
}

VkRenderPassBeginInfo RenderPass::create_default_render_pass_begin_info(size_t image_index)
{
    VkRenderPassBeginInfo info_begin_render_pass{};

    info_begin_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info_begin_render_pass.renderPass = render_pass;
    info_begin_render_pass.framebuffer = swapchain->get_swapchain_framebuffers()[image_index];
    info_begin_render_pass.renderArea.offset = {0, 0};
    info_begin_render_pass.renderArea.extent = swapchain->get_swapchain_extent();

    return info_begin_render_pass;
}

VkRenderPass RenderPass::get_render_pass() const
{
    return render_pass;
}

bool RenderPass::is_usable() const
{
    return usable;
}

//------------------------------------------------------------------------------------------------
//  "Default" render passes.
//------------------------------------------------------------------------------------------------

DefaultColorRenderPass::DefaultColorRenderPass(VkDevice* device, Swapchain* swapchain) : RenderPass(device, swapchain)
{
    VkAttachmentDescription attachment_color{};
    attachment_color.format = swapchain->get_swapchain_format().format;
    attachment_color.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    add_attachment(attachment_color);

    VkAttachmentReference* attachment_color_reference = new VkAttachmentReference{};
    attachment_color_reference->attachment = 0;
    attachment_color_reference->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_color{};
    subpass_color.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_color.colorAttachmentCount = 1;
    subpass_color.pColorAttachments = attachment_color_reference;

    add_subpass(subpass_color, attachment_color_reference);

    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    add_subpass_dependency(subpass_dependency);

    compile_render_pass();
}

DefaultColorRenderPass::~DefaultColorRenderPass() {}