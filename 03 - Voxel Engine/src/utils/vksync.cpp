#include "vksync.h"

#include <iostream>

#include "../renderer/vksetup.h"

fence::fence()
{
	VkFenceCreateInfo info_create_fence{};
	info_create_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
	VkResult r = vkCreateFence(get_device(), &info_create_fence, nullptr, &vk_fence);
	VERIFY_NORETURN(r, "Failed to create Vulkan fence.")
	
	usable = VERIFY_ASSIGN(r);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable)
	{
		std::cout << "[VK|INF] Created Vulkan fence." << std::endl;
	}
#endif
}

fence::fence(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo info_create_fence{};
	info_create_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info_create_fence.flags = flags;
	
	VkResult r = vkCreateFence(get_device(), &info_create_fence, nullptr, &vk_fence);
	VERIFY_NORETURN(r, "Failed to create Vulkan fence.")
	
	usable = VERIFY_ASSIGN(r);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable)
	{
		std::cout << "[VK|INF] Created Vulkan fence." << std::endl;
	}
#endif
}

fence::~fence()
{
	if(usable) vkDestroyFence(get_device(), vk_fence, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable)
	{
		std::cout << "[VK|INF] Destroyed Vulkan fence." << std::endl;
	}
#endif
}

VkFence fence::get_handle() const
{
	return vk_fence;
}

void fence::reset()
{
	if(!usable) return;
	vkResetFences(get_device(), 1, &vk_fence);
}

void fence::wait()
{
	if(!usable) return;
	vkWaitForFences(get_device(), 1, &vk_fence, VK_TRUE, UINT64_MAX);
}

semaphore::semaphore()
{
	VkSemaphoreCreateInfo info_create_semaphore{};
	info_create_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkResult r = vkCreateSemaphore(get_device(), &info_create_semaphore, nullptr, &vk_semaphore);
	VERIFY_NORETURN(r, "Failed to create Vulkan semaphore.")
	
	usable = VERIFY_ASSIGN(r);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable)
	{
		std::cout << "[VK|INF] Created Vulkan semaphore." << std::endl;
	}
#endif
}

semaphore::~semaphore()
{
	if(usable) vkDestroySemaphore(get_device(), vk_semaphore, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	if(usable)
	{
		std::cout << "[VK|INF] Destroyed Vulkan semaphore." << std::endl;
	}
#endif
}

VkSemaphore semaphore::get_handle() const
{
	return vk_semaphore;
}