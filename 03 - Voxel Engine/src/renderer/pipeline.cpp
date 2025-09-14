#include "pipeline.h"

#include "vksetup.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

static VkDynamicState default_dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

bool create_shader_module(VkShaderModule* shader_module, const char* filepath)
{
	//std::string module_path = std::filesystem::current_path().u8string() + filepath;
	std::ifstream reader(filepath, std::ios::binary);
	if(!reader.good())
	{
		std::cerr << "[VK|ERR] Could not locate or read file (for shader module creation):\n" << filepath << std::endl;
		return false;
	}
	
	reader.seekg(0, reader.end);
	size_t length = reader.tellg();
	reader.seekg(0, reader.beg);
	
	char* bytecode = new char[length];
	reader.read(bytecode, length);
	
	reader.close();
	
	VkShaderModuleCreateInfo info_module{};
	info_module.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info_module.codeSize = length;
	info_module.pCode = reinterpret_cast<const uint32_t*>(bytecode);
	
	VkShaderModule module;
	VkResult r = vkCreateShaderModule(get_device(), &info_module, nullptr, &module);
	VERIFY(r, "Failed to create Vulkan shader module.");
	
	*shader_module = module;
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Loaded shader module: " << filepath << std::endl;
#endif
	return true;
}

VkPipelineDepthStencilStateCreateInfo create_simple_depth_test_state()
{
	VkPipelineDepthStencilStateCreateInfo info_depth_stencil{};
	info_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info_depth_stencil.depthTestEnable = VK_TRUE;
	info_depth_stencil.depthWriteEnable = VK_TRUE;
	info_depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	info_depth_stencil.depthBoundsTestEnable = VK_FALSE;
	info_depth_stencil.stencilTestEnable = VK_FALSE;
	return info_depth_stencil;
}

VkVertexInputAttributeDescription create_vertex_input_attribute(uint32_t binding_index, uint32_t location, VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription attrib_desc{};
	attrib_desc.binding = binding_index;
	attrib_desc.location = location;
	attrib_desc.format = format;
	attrib_desc.offset = offset;
	
	return attrib_desc;
}

VkVertexInputBindingDescription create_vertex_input_binding(uint32_t binding_index, uint32_t stride, VkVertexInputRate input_rate)
{
	VkVertexInputBindingDescription info_vertex_input_binding{};
	info_vertex_input_binding.binding = binding_index;
	info_vertex_input_binding.stride = stride;
	info_vertex_input_binding.inputRate = input_rate;
	
	return info_vertex_input_binding;
}

VkPipelineDepthStencilStateCreateInfo getdefault_depth_stencil_state()
{
	VkPipelineDepthStencilStateCreateInfo info_depth_stencil{};
	info_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info_depth_stencil.depthTestEnable = VK_FALSE;
	info_depth_stencil.depthWriteEnable = VK_FALSE;
	info_depth_stencil.depthBoundsTestEnable = VK_FALSE;
	info_depth_stencil.stencilTestEnable = VK_FALSE;
	return info_depth_stencil;
}

VkPipelineDynamicStateCreateInfo getdefault_dynamic_state()
{
	VkPipelineDynamicStateCreateInfo info_dynamic_state{};
	info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info_dynamic_state.dynamicStateCount = 2;
	info_dynamic_state.pDynamicStates = default_dynamic_states;
	
	return info_dynamic_state;
}

VkPipelineInputAssemblyStateCreateInfo getdefault_input_assembly()
{
	VkPipelineInputAssemblyStateCreateInfo info_input_assembly{};
	info_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	info_input_assembly.primitiveRestartEnable = VK_FALSE;
	
	return info_input_assembly;
}

VkPipelineRasterizationStateCreateInfo getdefault_rasterization_state()
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

VkPipelineVertexInputStateCreateInfo get_vertex_input_state_noinput()
{
	VkPipelineVertexInputStateCreateInfo info_vertex_input{};
	info_vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info_vertex_input.vertexBindingDescriptionCount = 0;
	info_vertex_input.vertexAttributeDescriptionCount = 0;
	
	return info_vertex_input;
}

VkPipelineMultisampleStateCreateInfo get_multisample_state_none()
{
	VkPipelineMultisampleStateCreateInfo info_multisample{};
	info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info_multisample.sampleShadingEnable = VK_FALSE;
	info_multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	
	return info_multisample;
}

VkPipelineColorBlendAttachmentState get_color_blend_attachment_none()
{
	VkPipelineColorBlendAttachmentState attachment_color_blend{};
	attachment_color_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	attachment_color_blend.blendEnable = VK_FALSE;
	
	return attachment_color_blend;
}

pipeline::pipeline()
{
	info_depth_stencil_state = getdefault_depth_stencil_state();
	info_dynamic_state = getdefault_dynamic_state();
	info_vertex_input = get_vertex_input_state_noinput();
	info_input_assembly = getdefault_input_assembly();
	info_rasterizer = getdefault_rasterization_state();
	info_multisample = get_multisample_state_none();
	
	usable = false;
	layout_built = false;
}

pipeline::~pipeline()
{
	if(usable) vkDestroyPipeline(get_device(), vk_pipeline, nullptr);
	if(layout_built) vkDestroyPipelineLayout(get_device(), vk_pipeline_layout, nullptr);
	
	shaders.clear();
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan pipeline." << std::endl;
#endif
}

void pipeline::add_color_blend_state(VkPipelineColorBlendAttachmentState color_blend_state)
{
	info_color_blend_attachments.push_back(color_blend_state);
}

void pipeline::add_descriptor_set_layout(VkDescriptorSetLayout layout)
{
	descriptor_set_layouts.push_back(layout);
}

void pipeline::add_push_constant_range(VkShaderStageFlags shader_stage, uint32_t offset, uint32_t size)
{
	VkPushConstantRange range{};
	range.stageFlags = shader_stage;
	range.offset = offset;
	range.size = size;

	push_constant_ranges.push_back(range);
}

void pipeline::add_shader_module(VkShaderModule module, VkShaderStageFlagBits shader_stage, const char* entrypoint)
{
	pipeline_shader shader{module, shader_stage, entrypoint};
	shaders.push_back(shader);
}

void pipeline::add_viewport(VkViewport vp, VkRect2D scissor)
{
	pipeline_view pv;
	
	pv.viewport = vp;
	pv.scissor = scissor;
	
	viewports.push_back(pv);
}

bool pipeline::build(render_pass* rp)
{
	VkPipelineLayoutCreateInfo info_pipeline_layout{};
	info_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	
	if(descriptor_set_layouts.size() > 0)
	{
		info_pipeline_layout.setLayoutCount = descriptor_set_layouts.size();
		info_pipeline_layout.pSetLayouts = descriptor_set_layouts.data();
	}

	if(push_constant_ranges.size() > 0)
	{
		info_pipeline_layout.pushConstantRangeCount = push_constant_ranges.size();
		info_pipeline_layout.pPushConstantRanges = push_constant_ranges.data();
	}

	VkResult r = vkCreatePipelineLayout(get_device(), &info_pipeline_layout, nullptr, &vk_pipeline_layout);
	VERIFY(r, "Failed to create Vulkan pipeline layout.")
	
	layout_built = true;
	
	size_t num_shaders = shaders.size();
	if(num_shaders == 0)
	{
		std::cerr << "[VK|ERR] Attempted to build graphics pipeline with no shader modules." << std::endl;
		return false;
	}
	
	std::vector<VkPipelineShaderStageCreateInfo> info_shaders;
	info_shaders.resize(num_shaders);
	
	for(size_t i = 0; i < shaders.size(); i++)
	{
		info_shaders[i] = {};
		info_shaders[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info_shaders[i].stage = shaders[i].shader_stage;
		info_shaders[i].module = shaders[i].module;
		info_shaders[i].pName = shaders[i].entrypoint;
	}
	
	VkPipelineColorBlendStateCreateInfo info_color_blend{};
	info_color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	info_color_blend.logicOpEnable = VK_FALSE;
	info_color_blend.attachmentCount = info_color_blend_attachments.size();
	info_color_blend.pAttachments = info_color_blend_attachments.data();
	
	size_t num_viewports = viewports.size();
	
	std::vector<VkViewport> vk_viewports;
	std::vector<VkRect2D> scissors;
	
	vk_viewports.resize(num_viewports);
	scissors.resize(num_viewports);
	
	for(size_t i = 0; i < num_viewports; i++)
	{
		vk_viewports[i] = viewports[i].viewport;
		scissors[i] = viewports[i].scissor;
	}
	
	VkPipelineViewportStateCreateInfo info_viewport_state{};
	info_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	info_viewport_state.viewportCount = num_viewports;
	//info_viewport_state.pViewports = vk_viewports.data();
	info_viewport_state.scissorCount = num_viewports;
	//info_viewport_state.pScissors = scissors.data();
	
	VkGraphicsPipelineCreateInfo info_pipeline{};
	info_pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info_pipeline.stageCount = num_shaders;
	info_pipeline.pStages = info_shaders.data();
	info_pipeline.pVertexInputState = &info_vertex_input;
	info_pipeline.pInputAssemblyState = &info_input_assembly;
	info_pipeline.pViewportState = &info_viewport_state;
	info_pipeline.pRasterizationState = &info_rasterizer;
	info_pipeline.pMultisampleState = &info_multisample;
	info_pipeline.pDepthStencilState = &info_depth_stencil_state;
	info_pipeline.pColorBlendState = &info_color_blend;
	info_pipeline.pDynamicState = &info_dynamic_state;
	
	info_pipeline.layout = vk_pipeline_layout;
	info_pipeline.renderPass = rp->get_handle();
	info_pipeline.subpass = 0;
	
	r = vkCreateGraphicsPipelines(get_device(), VK_NULL_HANDLE, 1, &info_pipeline, nullptr, &vk_pipeline);
	VERIFY(r, "Failed to create Vulkan graphics pipeline.")

	usable = true;
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created Vulkan graphics pipeline." << std::endl;
#endif
	return usable;
}

VkPipeline pipeline::get_handle() const
{
	return vk_pipeline;
}

VkPipelineLayout pipeline::get_layout() const
{
	return vk_pipeline_layout;
}

pipeline_view pipeline::get_pipeline_view(size_t index) const
{
	if(index >= viewports.size())
	{
		std::cerr << "[VK|ERR] Attempted to access pipeline viewport index " << index << "(size is " << index << ")." << std::endl;
		return viewports[0];
	}
	
	return viewports[index];
}

void pipeline::set_pipeline_depth_stencil_state(VkPipelineDepthStencilStateCreateInfo depth_stencil_state)
{
	info_depth_stencil_state = depth_stencil_state;
}

void pipeline::set_pipeline_dynamic_state(VkPipelineDynamicStateCreateInfo dynamic_state)
{
	info_dynamic_state = dynamic_state;
}

void pipeline::set_pipeline_vertex_input_state(pipeline_vertex_input pvi)
{
	vertex_input = pvi;
	
	info_vertex_input.vertexBindingDescriptionCount = 1;
	info_vertex_input.vertexAttributeDescriptionCount = vertex_input.vertex_attribs.size();
	info_vertex_input.pVertexBindingDescriptions = &(vertex_input.vertex_binding);
	info_vertex_input.pVertexAttributeDescriptions = vertex_input.vertex_attribs.data();
}

void pipeline::set_pipeline_input_assembly_state(VkPipelineInputAssemblyStateCreateInfo input_assembly)
{
	info_input_assembly = input_assembly;
}

void pipeline::set_pipeline_rasterization_state(VkPipelineRasterizationStateCreateInfo rasterization_state)
{
	info_rasterizer = rasterization_state;
}

void pipeline::set_pipeline_multisample_state(VkPipelineMultisampleStateCreateInfo multisample_state)
{
	info_multisample = multisample_state;
}