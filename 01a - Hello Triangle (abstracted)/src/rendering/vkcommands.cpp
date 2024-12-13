#include "vkcommands.h"

#include <iostream>

CommandPool::CommandPool(VkDevice* device) : device(device)
{
    vksetup::queue_family_indices qf_indices = vksetup::find_physical_device_queue_families();

    VkCommandPoolCreateInfo info_create_pool{};

    info_create_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info_create_pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info_create_pool.queueFamilyIndex = qf_indices.graphics_index.value();

    init(info_create_pool);
}

CommandPool::CommandPool(VkDevice* device, VkCommandPoolCreateInfo info)
{
    init(info);
}

CommandPool::~CommandPool()
{
    vkDestroyCommandPool(*device, command_pool, nullptr);
}

bool CommandPool::allocate_command_buffer(VkCommandBuffer* buffer, bool is_primary_command_buffer)
{
    VkCommandBufferAllocateInfo info_alloc_buffer{};

    info_alloc_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info_alloc_buffer.commandPool = command_pool;
    info_alloc_buffer.level = is_primary_command_buffer ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    info_alloc_buffer.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(*device, &info_alloc_buffer, buffer);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to allocate command buffer." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

VkCommandPool CommandPool::get_command_pool() const
{
    return command_pool;
}

void CommandPool::init(VkCommandPoolCreateInfo info)
{
    VkResult result = vkCreateCommandPool(*device, &info, nullptr, &command_pool);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create command pool handle." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        usable = false;
    }

    usable = true;
}

bool CommandPool::is_usable() const
{
    return usable;
}