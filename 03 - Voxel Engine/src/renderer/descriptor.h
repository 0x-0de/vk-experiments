#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#include "vksetup.h"

#include "../utils/alloc.h"

#include "texture.h"

#define DESCRIPTOR_BINDING_TYPE_BUFFER 0
#define DESCRIPTOR_BINDING_TYPE_IMAGE 1

struct descriptor_binding
{
	uint8_t type;
	VkShaderStageFlags shader_stage;

	uint32_t size;
	size_t buffer_index;

	texture* texture;
};

class descriptor
{
	public:
		descriptor(uint32_t descriptor_count);
		~descriptor();

		void add_descriptor_binding_buffer(uint32_t size, VkShaderStageFlags shader_stage);
		void add_descriptor_binding_sampler(texture* texture, VkShaderStageFlags shader_stage);

		bool build();
		
		static bool create_descriptor_pool(VkDescriptorPool* pool, std::vector<VkDescriptorType> types, uint32_t descriptor_count);
		static VkDescriptorSetLayoutBinding create_descriptor_set_binding(uint32_t binding_index, VkDescriptorType type, VkShaderStageFlags shader_stage);
		static bool create_descriptor_set_layout(VkDescriptorSetLayout* layout, std::vector<VkDescriptorSetLayoutBinding> bindings);
		
		VkDescriptorSet get_descriptor_set(size_t index) const;
		VkDescriptorSetLayout get_descriptor_set_layout() const;
		uint32_t get_descriptor_count() const;

		void place_data(uint32_t descriptor_set, uint32_t binding_index, VkDeviceSize offset, VkDeviceSize size, void* data);
	private:
		bool allocate_descriptor_sets();
		std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings();
		bool create_internal_descriptor_pool(std::vector<VkDescriptorSetLayoutBinding> bindings);
		void configure_descriptor_sets();

		uint32_t descriptor_count;

		bool usable;

		VkDescriptorPool descriptor_pool;
		VkDescriptorSetLayout set_layout;

		std::vector<VkDescriptorSet> descriptor_sets;
		std::vector<descriptor_binding> descriptor_bindings;

		std::vector<alloc::buffer> buffers;
};

#endif