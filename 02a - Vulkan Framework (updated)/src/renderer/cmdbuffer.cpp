#include "cmdbuffer.h"

#include <iostream>

#include "vksetup.h"

bool create_command_pool(VkCommandPool* command_pool, uint32_t queue_family_index)
{
	VkCommandPoolCreateInfo info_command_pool{};
	info_command_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info_command_pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info_command_pool.queueFamilyIndex = queue_family_index;
	
	VkResult r = vkCreateCommandPool(get_device(), &info_command_pool, nullptr, command_pool);
	VERIFY(r, "Failed to create command pool.")
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created command pool for queue family index: " << queue_family_index << std::endl;
#endif
	return true;
}

command_buffer::command_buffer(VkCommandPool command_pool)
{
	VkCommandBufferAllocateInfo info_alloc_buffer{};
	info_alloc_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info_alloc_buffer.commandPool = command_pool;
	info_alloc_buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info_alloc_buffer.commandBufferCount = 1;
	
	VkResult r = vkAllocateCommandBuffers(get_device(), &info_alloc_buffer, &vk_command_buffer);
	VERIFY_NORETURN(r, "Failed to create command buffer.")
	
	usable = VERIFY_ASSIGN(r);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable) std::cout << "[VK|INF] Allocated a command buffer." << std::endl;
#endif
}

command_buffer::~command_buffer() {}

bool command_buffer::begin_recording()
{
	VkCommandBufferBeginInfo info_begin{};
	info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkResult r = vkBeginCommandBuffer(vk_command_buffer, &info_begin);
	VERIFY(r, "Failed to begin recording Vulkan command buffer.")
	
	return true;
}

void command_buffer::begin_render_pass(render_pass* rp, VkFramebuffer framebuffer, VkExtent2D extent)
{
	VkRenderPassBeginInfo info_begin_render_pass{};
	info_begin_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info_begin_render_pass.renderPass = rp->get_handle();
	info_begin_render_pass.framebuffer = framebuffer;
	info_begin_render_pass.renderArea.offset = {0, 0};
	info_begin_render_pass.renderArea.extent = extent;

	VkClearValue clear_values[2];

	clear_values[0] = {{{0, 0, 0, 1}}};
	clear_values[1] = {};
	clear_values[1].depthStencil = {1, 0};

	info_begin_render_pass.clearValueCount = 2;
	info_begin_render_pass.pClearValues = clear_values;
	
	vkCmdBeginRenderPass(vk_command_buffer, &info_begin_render_pass, VK_SUBPASS_CONTENTS_INLINE);
}

void command_buffer::bind_descriptor_set(VkPipelineLayout layout, VkDescriptorSet descriptor_set)
{
	vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptor_set, 0, nullptr);
}

void command_buffer::bind_index_buffer(VkBuffer buffer, uint32_t offset)
{
	vkCmdBindIndexBuffer(vk_command_buffer, buffer, offset, VK_INDEX_TYPE_UINT16);
}

void command_buffer::bind_pipeline(pipeline* p)
{
	vkCmdBindPipeline(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->get_handle());
}

void command_buffer::bind_vertex_buffer(VkBuffer buffer, uint32_t offset)
{
	VkBuffer buffers[] = {buffer};
	VkDeviceSize offsets[] = {offset};
	
	vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, buffers, offsets);
}

void command_buffer::draw(uint32_t num_vertices, uint32_t num_instances)
{
	draw(num_vertices, num_instances, 0, 0);
}

void command_buffer::draw(uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance)
{
	vkCmdDraw(vk_command_buffer, num_vertices, num_instances, first_vertex, first_instance);
}

void command_buffer::draw_indexed(uint32_t num_indices, uint32_t num_instances)
{
	draw_indexed(num_indices, num_instances, 0, 0, 0);
}

void command_buffer::draw_indexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index, uint32_t index_add, uint32_t first_instance)
{
	vkCmdDrawIndexed(vk_command_buffer, num_indices, num_instances, first_index, index_add, first_instance);
}

bool command_buffer::end_recording()
{
	VkResult r = vkEndCommandBuffer(vk_command_buffer);
	VERIFY(r, "Failed to end recording Vulkan command buffer.")
	
	return true;
}

void command_buffer::end_render_pass()
{
	vkCmdEndRenderPass(vk_command_buffer);
}

VkCommandBuffer command_buffer::get_handle() const
{
	return vk_command_buffer;
}

void command_buffer::reset()
{
	vkResetCommandBuffer(vk_command_buffer, 0);
}

void command_buffer::set_viewport(pipeline_view view)
{
	set_viewport(view.viewport, view.scissor);
}

void command_buffer::set_viewport(VkViewport viewport, VkRect2D scissor)
{
	vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
}