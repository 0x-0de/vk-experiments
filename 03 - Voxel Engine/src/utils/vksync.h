#ifndef _VKSYNC_H_
#define _VKSYNC_H_

#include "../../lib/vulkan/vulkan.hpp"

class fence
{
	public:
		fence();
		fence(VkFenceCreateFlags flags);
		~fence();
		
		VkFence get_handle() const;
		
		void reset();
		void wait();
	private:
		VkFence vk_fence;
		
		bool usable;
};

class semaphore
{
	public:
		semaphore();
		~semaphore();
		
		VkSemaphore get_handle() const;
	private:
		VkSemaphore vk_semaphore;
		
		bool usable;
};

#endif