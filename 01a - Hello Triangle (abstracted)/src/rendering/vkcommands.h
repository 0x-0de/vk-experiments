#ifndef _VK_COMMANDS_H_
#define _VK_COMMANDS_H_

#include "vksetup.h"

class CommandPool
{
    public:
        CommandPool(VkDevice* device);
        CommandPool(VkDevice* device, VkCommandPoolCreateInfo info);
        ~CommandPool();

        bool allocate_command_buffer(VkCommandBuffer* buffer, bool is_primary_command_buffer);

        VkCommandPool get_command_pool() const;

        bool is_usable() const;
    private:
        VkDevice* device;
        VkCommandPool command_pool;

        bool usable;

        void init(VkCommandPoolCreateInfo info);
};

#endif