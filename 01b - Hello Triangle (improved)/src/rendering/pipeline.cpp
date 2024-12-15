#include "pipeline.h"

#include <iostream>

#define DEFAULT_DYNAMIC_STATES_SIZE 2
static const VkDynamicState default_dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

VkPipelineDynamicStateCreateInfo create_default_pipeline_dynamic_state()
{
    VkPipelineDynamicStateCreateInfo info_dynamic_state{};
    info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info_dynamic_state.dynamicStateCount = DEFAULT_DYNAMIC_STATES_SIZE;
    info_dynamic_state.pDynamicStates = default_dynamic_states;

    return info_dynamic_state;
}

VkPipelineVertexInputStateCreateInfo create_default_pipeline_vertex_input()
{
    VkPipelineVertexInputStateCreateInfo info_vertex_input{};
    info_vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info_vertex_input.vertexBindingDescriptionCount = 0;
    info_vertex_input.vertexAttributeDescriptionCount = 0;

    return info_vertex_input;
}

VkPipelineInputAssemblyStateCreateInfo create_default_pipeline_input_assembly()
{
    VkPipelineInputAssemblyStateCreateInfo info_input_assembly{};
    info_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info_input_assembly.primitiveRestartEnable = VK_FALSE;

    return info_input_assembly;
}

VkPipelineRasterizationStateCreateInfo create_default_pipeline_rasterization_state()
{
    VkPipelineRasterizationStateCreateInfo info_rasterizer{};
    info_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    info_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    info_rasterizer.lineWidth = 1;
    info_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    info_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info_rasterizer.depthBiasEnable = VK_FALSE;

    return info_rasterizer;
}

VkPipelineMultisampleStateCreateInfo create_default_pipeline_multisample_state()
{
    VkPipelineMultisampleStateCreateInfo info_multisample{};
    info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info_multisample.sampleShadingEnable = VK_FALSE;
    info_multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return info_multisample;
}

VkPipelineColorBlendAttachmentState create_default_color_blend_overwrite()
{
    VkPipelineColorBlendAttachmentState attachment_color_blend{};
    attachment_color_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment_color_blend.blendEnable = VK_FALSE;

    return attachment_color_blend;
}

VkPipelineColorBlendStateCreateInfo create_default_color_blend_state(VkPipelineColorBlendAttachmentState* attachment)
{
    VkPipelineColorBlendStateCreateInfo info_color_blending{};
    info_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info_color_blending.logicOpEnable = VK_FALSE;
    info_color_blending.attachmentCount = 1;
    info_color_blending.pAttachments = attachment;

    return info_color_blending;
}

VkPipelineLayoutCreateInfo create_default_pipeline_layout()
{
    VkPipelineLayoutCreateInfo info_pipeline_layout{};
    info_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info_pipeline_layout.setLayoutCount = 0; //Default value - optional.

    return info_pipeline_layout;
}

Pipeline::Pipeline(VkDevice* device, RenderPass* render_pass) : device(device), render_pass(render_pass) {}

Pipeline::~Pipeline()
{
    vkDestroyPipeline(*device, pipeline, nullptr);
    vkDestroyPipelineLayout(*device, pipeline_layout, nullptr);
}

void Pipeline::add_color_blend_attachment(VkPipelineColorBlendAttachmentState attachment)
{
    color_blend_attachments.push_back(attachment);
}

void Pipeline::add_shader_module(VkPipelineShaderStageCreateInfo module_info)
{
    info_shader_modules.push_back(module_info);
}

void Pipeline::clear_data()
{
    info_shader_modules.clear();
    color_blend_attachments.clear();
}

void Pipeline::compile_pipeline()
{
    //Needs to be done after 'set viewport.'
    VkPipelineViewportStateCreateInfo info_viewport{};
    info_viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info_viewport.viewportCount = 1;
    info_viewport.pViewports = &viewport;
    info_viewport.scissorCount = 1;
    info_viewport.pScissors = &viewport_scissor;

    VkResult result = vkCreatePipelineLayout(*device, &info_pipeline_layout, nullptr, &pipeline_layout);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create graphics pipeline layout." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;

        usable = false;
        return;
    }

    VkGraphicsPipelineCreateInfo info_pipeline{};
    info_pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info_pipeline.stageCount = info_shader_modules.size();
    info_pipeline.pStages = info_shader_modules.data();
    info_pipeline.pVertexInputState = &info_vertex_input;
    info_pipeline.pInputAssemblyState = &info_input_assembly;
    info_pipeline.pViewportState = &info_viewport;
    info_pipeline.pRasterizationState = &info_rasterizer;
    info_pipeline.pMultisampleState = &info_multisample;
    info_pipeline.pDepthStencilState = nullptr;
    info_pipeline.pColorBlendState = &info_color_blending;
    info_pipeline.pDynamicState = &info_dynamic_state;
    info_pipeline.layout = pipeline_layout;
    info_pipeline.renderPass = render_pass->get_render_pass();
    info_pipeline.subpass = 0;

    result = vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &info_pipeline, nullptr, &pipeline);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create graphics pipeline." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;

        usable = false;
        return;
    }

    usable = true;
}

VkPipeline Pipeline::get_pipeline() const
{
    return pipeline;
}

VkPipelineLayout Pipeline::get_pipeline_layout() const
{
    return pipeline_layout;
}

void Pipeline::set_color_blending(VkPipelineColorBlendStateCreateInfo info)
{
    info_color_blending = info;
}

void Pipeline::set_dynamic_state(VkPipelineDynamicStateCreateInfo info)
{
    info_dynamic_state = info;
}

void Pipeline::set_input_assembly(VkPipelineInputAssemblyStateCreateInfo info)
{
    info_input_assembly = info;
}

void Pipeline::set_multisample_state(VkPipelineMultisampleStateCreateInfo info)
{
    info_multisample = info;
}

void Pipeline::set_pipeline_layout(VkPipelineLayoutCreateInfo info)
{
    info_pipeline_layout = info;
}

void Pipeline::set_rasterizer(VkPipelineRasterizationStateCreateInfo info)
{
    info_rasterizer = info;
}

void Pipeline::set_vertex_input(VkPipelineVertexInputStateCreateInfo info)
{
    info_vertex_input = info;
}

void Pipeline::set_viewport(VkViewport vp, VkRect2D vp_scissor)
{
    viewport = vp;
    viewport_scissor = vp_scissor;
}

//--------------------------------------------------------------------------------------

DefaultColorPipeline::DefaultColorPipeline(VkDevice* device, RenderPass* render_pass, Swapchain* swapchain) : Pipeline(device, render_pass)
{
    VkPipelineColorBlendAttachmentState color_attachment = create_default_color_blend_overwrite();
    add_color_blend_attachment(color_attachment);

    info_color_blending = VkPipelineColorBlendStateCreateInfo{};
    info_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info_color_blending.logicOpEnable = VK_FALSE;
    info_color_blending.attachmentCount = color_blend_attachments.size();
    info_color_blending.pAttachments = color_blend_attachments.data();

    set_dynamic_state(create_default_pipeline_dynamic_state());
    set_input_assembly(create_default_pipeline_input_assembly());
    set_multisample_state(create_default_pipeline_multisample_state());
    set_pipeline_layout(create_default_pipeline_layout());
    set_rasterizer(create_default_pipeline_rasterization_state());
    set_vertex_input(create_default_pipeline_vertex_input());

    usable = swapchain->get_swapchain_viewport_and_scissor(&viewport, &viewport_scissor);
    if(!usable)
    {
        std::cerr << "[ERR] Swapchain passed into DefaultColorPipeline is unusable." << std::endl;
    }
}

DefaultColorPipeline::~DefaultColorPipeline() {}