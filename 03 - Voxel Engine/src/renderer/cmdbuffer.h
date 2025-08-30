#ifndef _COMMAND_BUFFER_H_
#define _COMMAND_BUFFER_H_

#include "pipeline.h"

bool create_command_pool(VkCommandPool* command_pool, uint32_t queue_family_index);

class command_buffer
{
	public:
		command_buffer(VkCommandPool command_pool);
		~command_buffer();
	
		bool begin_recording();
		static bool begin_recording_onetime(VkCommandBuffer command_buffer);
		void begin_render_pass(render_pass* rp, VkFramebuffer framebuffer, VkExtent2D extent);
		
		void bind_descriptor_set(VkPipelineLayout layout, VkDescriptorSet descriptor_set);
		void bind_index_buffer(VkBuffer buffer, uint32_t offset);
		void bind_pipeline(pipeline* p);
		void bind_vertex_buffer(VkBuffer buffer, uint32_t offset);
		
		void draw(uint32_t num_vertices, uint32_t num_instances);
		void draw(uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
		
		void draw_indexed(uint32_t num_indices, uint32_t num_instances);
		void draw_indexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index, uint32_t index_add, uint32_t first_instance);
		
		bool end_recording();
		void end_render_pass();
		
		VkCommandBuffer get_handle() const;
		
		void reset();
		
		void set_viewport(pipeline_view view);
		void set_viewport(VkViewport viewport, VkRect2D scissor);
	private:
		VkCommandBuffer vk_command_buffer;
		bool usable;
};

#endif