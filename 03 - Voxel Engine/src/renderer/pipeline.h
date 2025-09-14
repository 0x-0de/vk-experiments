#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "../../lib/vulkan/vulkan.hpp"

#define DEFAULT_SHADER_ENTRYPOINT "main"

#include "renderpass.h"

bool create_shader_module(VkShaderModule* shader_module, const char* filepath);

VkPipelineDepthStencilStateCreateInfo create_simple_depth_test_state();

VkVertexInputAttributeDescription create_vertex_input_attribute(uint32_t binding_index, uint32_t location, VkFormat format, uint32_t offset);
VkVertexInputBindingDescription create_vertex_input_binding(uint32_t binding_index, uint32_t stride, VkVertexInputRate input_rate);

VkPipelineDepthStencilStateCreateInfo getdefault_depth_stencil_state();
VkPipelineDynamicStateCreateInfo getdefault_dynamic_state();
VkPipelineInputAssemblyStateCreateInfo getdefault_input_assembly();
VkPipelineRasterizationStateCreateInfo getdefault_rasterization_state();

VkPipelineVertexInputStateCreateInfo get_vertex_input_state_noinput();
VkPipelineMultisampleStateCreateInfo get_multisample_state_none();
VkPipelineColorBlendAttachmentState get_color_blend_attachment_none();

struct pipeline_shader
{
	VkShaderModule module;
	VkShaderStageFlagBits shader_stage;
	const char* entrypoint;
};

struct pipeline_view
{
	VkViewport viewport;
	VkRect2D scissor;
};

struct pipeline_vertex_input
{
	VkVertexInputBindingDescription vertex_binding;
	std::vector<VkVertexInputAttributeDescription> vertex_attribs;
};

class pipeline
{
	public:
		pipeline();
		~pipeline();
		
		void add_color_blend_state(VkPipelineColorBlendAttachmentState color_blend_state);
		void add_descriptor_set_layout(VkDescriptorSetLayout layout);
		void add_push_constant_range(VkShaderStageFlags shader_stage, uint32_t offset, uint32_t size);
		void add_shader_module(VkShaderModule module, VkShaderStageFlagBits shader_stage, const char* entrypoint);
		void add_viewport(VkViewport vp, VkRect2D scissor);
		
		bool build(render_pass* rp);
		
		VkPipeline get_handle() const;
		VkPipelineLayout get_layout() const;
		
		pipeline_view get_pipeline_view(size_t index) const;
		
		void set_pipeline_depth_stencil_state(VkPipelineDepthStencilStateCreateInfo depth_stencil_state);
		void set_pipeline_dynamic_state(VkPipelineDynamicStateCreateInfo dynamic_state);
		void set_pipeline_vertex_input_state(pipeline_vertex_input pvi);
		void set_pipeline_input_assembly_state(VkPipelineInputAssemblyStateCreateInfo input_assembly);
		void set_pipeline_rasterization_state(VkPipelineRasterizationStateCreateInfo rasterization_state);
		void set_pipeline_multisample_state(VkPipelineMultisampleStateCreateInfo multisample_state);
	private:
		VkPipeline vk_pipeline;
		VkPipelineLayout vk_pipeline_layout;
		
		VkPipelineDepthStencilStateCreateInfo info_depth_stencil_state;
		VkPipelineDynamicStateCreateInfo info_dynamic_state;
		VkPipelineVertexInputStateCreateInfo info_vertex_input;
		VkPipelineInputAssemblyStateCreateInfo info_input_assembly;
		VkPipelineRasterizationStateCreateInfo info_rasterizer;
		VkPipelineMultisampleStateCreateInfo info_multisample;
		
		std::vector<pipeline_shader> shaders;
		std::vector<pipeline_view> viewports;
		
		std::vector<VkPipelineColorBlendAttachmentState> info_color_blend_attachments;
		std::vector<VkPushConstantRange> push_constant_ranges;
		
		pipeline_vertex_input vertex_input;
		std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
		
		bool usable;
		bool layout_built;
};

#endif