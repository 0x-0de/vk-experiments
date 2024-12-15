#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "renderpass.h"

class Pipeline
{
    public:
        Pipeline(VkDevice* device, RenderPass* render_pass);
        virtual ~Pipeline();

        void add_color_blend_attachment(VkPipelineColorBlendAttachmentState attachment);
        void add_shader_module(VkPipelineShaderStageCreateInfo module_info);

        void compile_pipeline();
    
        VkPipeline get_pipeline() const;
        VkPipelineLayout get_pipeline_layout() const;

        void set_color_blending(VkPipelineColorBlendStateCreateInfo info);
        void set_dynamic_state(VkPipelineDynamicStateCreateInfo info);
        void set_input_assembly(VkPipelineInputAssemblyStateCreateInfo info);
        void set_multisample_state(VkPipelineMultisampleStateCreateInfo info);
        void set_pipeline_layout(VkPipelineLayoutCreateInfo info);
        void set_rasterizer(VkPipelineRasterizationStateCreateInfo info);
        void set_vertex_input(VkPipelineVertexInputStateCreateInfo info);

        void set_viewport(VkViewport vp, VkRect2D vp_scissor);
    protected:
        VkDevice* device;
        RenderPass* render_pass;

        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;

        std::vector<VkPipelineShaderStageCreateInfo> info_shader_modules;
        VkPipelineDynamicStateCreateInfo info_dynamic_state;
        VkPipelineVertexInputStateCreateInfo info_vertex_input;
        VkPipelineInputAssemblyStateCreateInfo info_input_assembly;
        VkPipelineRasterizationStateCreateInfo info_rasterizer;
        VkPipelineMultisampleStateCreateInfo info_multisample;
        VkPipelineColorBlendStateCreateInfo info_color_blending;
        VkPipelineLayoutCreateInfo info_pipeline_layout;

        std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;

        VkViewport viewport;
        VkRect2D viewport_scissor;

        bool usable;

        void clear_data();
};

class DefaultColorPipeline : public Pipeline
{
    public:
        DefaultColorPipeline(VkDevice* device, RenderPass* render_pass, Swapchain* swapchain);
        ~DefaultColorPipeline();
};

#endif