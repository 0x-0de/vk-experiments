#include "swapchain.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "vksetup.h"

std::string swapchain_properties_to_string(swapchain* sc)
{
	std::string str = "[VK|INF] Swap chain properties:\n";
	str += "\tFormat: \n";

	VkSurfaceFormatKHR format = sc->get_format();

	str += "\t\tInternal format: " + std::to_string(format.format) + "\n";

	VkColorSpaceKHR color_space = format.colorSpace;
	str += "\t\tColor space: ";
	switch(format.colorSpace)
	{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
			str += "SRGB Nonlinear\n";
			break;
		default:
			str += "External color space\n";
			break;
	}

	str += "\tPresesntation mode: ";
	switch(sc->get_presentation_mode())
	{
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			str += "Immediate\n";
			break;
		case VK_PRESENT_MODE_MAILBOX_KHR:
			str += "Mailbox\n";
			break;
		case VK_PRESENT_MODE_FIFO_KHR:
			str += "FIFO\n";
			break;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			str += "Relaxed FIFO\n";
			break;
	}

	str += "\tExtent: ";
	VkExtent2D extent = sc->get_extent();
	str += std::to_string(extent.width) + ", " + std::to_string(extent.height) + "\n";

	str += "\tNumber of images (frames): " + std::to_string(sc->get_image_count());

	return str;
}

swapchain_properties query_swapchain_properties(VkPhysicalDevice device)
{
	VkSurfaceKHR surface = get_window_surface();
    swapchain_properties properties;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &properties.capabilities);

    uint32_t format_count, present_mode_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if(format_count != 0)
    {
        properties.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, properties.formats.data());
    }

    if(present_mode_count != 0)
    {
        properties.presentation_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, properties.presentation_modes.data());
    }

    return properties;
}

VkSurfaceFormatKHR choose_swapchain_format(swapchain_properties props)
{
    for(auto format : props.formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return props.formats[0];
}

VkPresentModeKHR choose_swapchain_present_mode(swapchain_properties props)
{
    for(auto present_mode : props.presentation_modes)
    {
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR; //Garunteed to be supported.
}

VkExtent2D choose_swapchain_extent(GLFWwindow* window, swapchain_properties props)
{
    auto& cap = props.capabilities;
    if(cap.currentExtent.width != UINT32_MAX)
        return cap.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D extent = {(uint32_t) width, (uint32_t) height};
        extent.width = std::clamp(extent.width, cap.minImageExtent.width, cap.maxImageExtent.width);
        extent.height = std::clamp(extent.height, cap.minImageExtent.height, cap.maxImageExtent.height);

        return extent;
    }
}

bool swapchain::create_framebuffer(VkFramebuffer* framebuffer, render_pass* rp, std::vector<VkImageView> views, VkExtent2D extent)
{	
	VkFramebufferCreateInfo info_framebuffer{};
	info_framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info_framebuffer.renderPass = rp->get_handle();
	info_framebuffer.attachmentCount = views.size();
	info_framebuffer.pAttachments = views.data();
	info_framebuffer.width = extent.width;
	info_framebuffer.height = extent.height;
	info_framebuffer.layers = 1;

	VkResult r = vkCreateFramebuffer(get_device(), &info_framebuffer, nullptr, framebuffer);
	VERIFY(r, "Failed to create swapchain framebuffer.")
	return true;
}

swapchain::swapchain(GLFWwindow* window) : window(window)
{
	usable = create_swap_chain();
	if(!init_sync_objects()) usable = false;
}

swapchain::~swapchain()
{	
	for(uint32_t i = 0; i < image_views.size(); i++)
	{
		delete frame_finished[i];
		
		delete render_finished[i];
		//delete image_retrieved[i];
	}

	delete image_retrieved;

	destroy_swap_chain();
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan swapchain." << std::endl;
#endif
}

void swapchain::add_swapchain_render_target(VkImageView render_target)
{
	additional_render_targets.push_back(render_target);
}

void swapchain::add_swapchain_resize_callback(void (*callback)(swapchain* sc))
{
	swapchain_resize_callbacks.push_back(callback);
}

void swapchain::clear_swapchain_render_targets()
{
	additional_render_targets.clear();
}

bool swapchain::create_framebuffers(render_pass* rp)
{
	framebuffers.resize(image_views.size());
	
	for(uint32_t i = 0; i < framebuffers.size(); i++)
	{
		std::vector<VkImageView> attachments;
		attachments.resize(additional_render_targets.size() + 1);

		attachments[0] = image_views[i];
		for(size_t j = 0; j < additional_render_targets.size(); j++)
			attachments[j + 1] = additional_render_targets[j];

		VkFramebufferCreateInfo info_framebuffer{};
		info_framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info_framebuffer.renderPass = rp->get_handle();
		info_framebuffer.attachmentCount = attachments.size();
		info_framebuffer.pAttachments = attachments.data();
		info_framebuffer.width = extent.width;
		info_framebuffer.height = extent.height;
		info_framebuffer.layers = 1;

		VkResult r = vkCreateFramebuffer(get_device(), &info_framebuffer, nullptr, framebuffers.data() + i);
		VERIFY(r, "Failed to create swapchain framebuffer.")
	}
	
	refresh_rp = rp;
	return true;
}

bool swapchain::create_swap_chain()
{
	swapchain_properties properties = query_swapchain_properties(get_selected_physical_device());
	
	format = choose_swapchain_format(properties);
	presentation_mode = choose_swapchain_present_mode(properties);
	extent = choose_swapchain_extent(window, properties);
	
	uint32_t max_image_count = properties.capabilities.maxImageCount;
	
	image_count = properties.capabilities.minImageCount + 1;
	if(image_count > max_image_count && max_image_count != 0)
		image_count = max_image_count;
	
	VkSwapchainCreateInfoKHR info_swapchain{};
	info_swapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info_swapchain.surface = get_window_surface();
	info_swapchain.presentMode = presentation_mode;
	info_swapchain.minImageCount = image_count;
	info_swapchain.imageFormat = format.format;
	info_swapchain.imageColorSpace = format.colorSpace;
	info_swapchain.imageExtent = extent;
	info_swapchain.imageArrayLayers = 1;
	info_swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	queue_family qf = find_physical_device_queue_families(get_selected_physical_device());
	uint32_t qfi[] = {qf.queue_index_graphics.value(), qf.queue_index_present.value()};

	if(qfi[0] == qfi[1])
		info_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	else
	{
		info_swapchain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info_swapchain.queueFamilyIndexCount = 2;
		info_swapchain.pQueueFamilyIndices = qfi;
	}
	
	info_swapchain.preTransform = properties.capabilities.currentTransform;
	info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info_swapchain.clipped = VK_TRUE;
	info_swapchain.oldSwapchain = VK_NULL_HANDLE;
	
	VkResult r = vkCreateSwapchainKHR(get_device(), &info_swapchain, nullptr, &vk_swapchain);
	VERIFY(r, "Failed to create Vulkan swapchain.");
	
	bool success = true;
	if(!init_image_views()) success = false;
	
	if(success) success = VERIFY_ASSIGN(r);
	
	current_image = 0;

	viewport = get_default_viewport();
	scissor = get_full_scissor();
#ifdef DEBUG_PRINT_SUCCESS
	vkGetSwapchainImagesKHR(get_device(), vk_swapchain, &image_count, nullptr);
	if(success)
	{
		std::cout << "[VK|INF] Created Vulkan swapchain." << std::endl;
		std::cout << swapchain_properties_to_string(this) << std::endl;
	}
#endif
	return success;
}

void swapchain::destroy_swap_chain()
{
	for(uint32_t i = 0; i < framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(get_device(), framebuffers[i], nullptr);
	}

	for(uint32_t i = 0; i < image_views.size(); i++)
	{
		vkDestroyImageView(get_device(), image_views[i], nullptr);
	}
	
	vkDestroySwapchainKHR(get_device(), vk_swapchain, nullptr);
}

uint32_t swapchain::get_current_image_index() const
{
	return current_image;
}

VkViewport swapchain::get_default_viewport() const
{
	VkViewport viewport{};

	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float) extent.width;
	viewport.height = (float) extent.height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	
	return viewport;
}

VkExtent2D swapchain::get_extent() const
{
	return extent;
}

VkSurfaceFormatKHR swapchain::get_format() const
{
	return format;
}

VkFramebuffer swapchain::get_framebuffer(size_t index)
{
	return framebuffers[index];
}

VkRect2D swapchain::get_full_scissor() const
{
	VkRect2D scissor{};
	
	scissor.offset = {0, 0};
	scissor.extent = extent;
	
	return scissor;
}

VkSwapchainKHR swapchain::get_handle() const
{
	return vk_swapchain;
}

uint32_t swapchain::get_image_count() const
{
	return image_count;
}

VkPresentModeKHR swapchain::get_presentation_mode() const
{
	return presentation_mode;
}

VkRect2D swapchain::get_scissor() const
{
	return scissor;
}

fence* swapchain::get_sync_fence_frame_finished(uint32_t frame_index)
{
	return frame_finished[frame_index];
}

semaphore* swapchain::get_sync_semaphore_render_finished(uint32_t frame_index)
{
	return render_finished[frame_index];
}

VkViewport swapchain::get_viewport() const
{
	return viewport;
}

bool swapchain::init_image_views()
{
	uint32_t image_count = 0;
	
	vkGetSwapchainImagesKHR(get_device(), vk_swapchain, &image_count, nullptr);
	images.resize(image_count);
	image_views.resize(image_count);
	vkGetSwapchainImagesKHR(get_device(), vk_swapchain, &image_count, images.data());
	
	for(uint32_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo info_image_view{};
		info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info_image_view.image = images[i];
		info_image_view.format = format.format;
		info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info_image_view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info_image_view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info_image_view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info_image_view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info_image_view.subresourceRange.baseMipLevel = 0;
		info_image_view.subresourceRange.levelCount = 1;
		info_image_view.subresourceRange.baseArrayLayer = 0;
		info_image_view.subresourceRange.layerCount = 1;
		
		VkResult r = vkCreateImageView(get_device(), &info_image_view, nullptr, &image_views[i]);
		VERIFY(r, "Failed to create swapchain image view.")
	}
	
	return true;
}

bool swapchain::init_sync_objects()
{
	uint32_t image_count = 0;
	
	vkGetSwapchainImagesKHR(get_device(), vk_swapchain, &image_count, nullptr);
	
	//image_retrieved.resize(image_count);
	render_finished.resize(image_count);
	
	frame_finished.resize(image_count);
	
	for(uint32_t i = 0; i < image_count; i++)
	{
		//image_retrieved[i] = new semaphore();
		render_finished[i] = new semaphore();
		
		frame_finished[i] = new fence(VK_FENCE_CREATE_SIGNALED_BIT);
	}

	image_retrieved = new fence();
	
	return true;
}

bool swapchain::is_usable() const
{
	return usable;
}

void swapchain::image_present(VkQueue queue)
{
	VkSemaphore wait_semaphores[] = {render_finished[current_image]->get_handle()};
	
	VkPresentInfoKHR info_present{};
	info_present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info_present.waitSemaphoreCount = 1;
	info_present.pWaitSemaphores = wait_semaphores;
	
	VkSwapchainKHR swapchains[] = {vk_swapchain};

	info_present.swapchainCount = 1;
	info_present.pSwapchains = swapchains;
	info_present.pImageIndices = &current_image;
	
	vkQueuePresentKHR(queue, &info_present);
}

bool swapchain::image_render(VkQueue queue, command_buffer* buffer)
{
	VkSubmitInfo info_submit{};
	info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//VkSemaphore wait_semaphores[] = {image_retrieved[retrieval_index]->get_handle()};
	//VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	info_submit.waitSemaphoreCount = 0;
	//info_submit.pWaitSemaphores = wait_semaphores;
	//info_submit.pWaitDstStageMask = wait_stages;
	
	VkCommandBuffer command_buffers[] = {buffer->get_handle()};

	info_submit.commandBufferCount = 1;
	info_submit.pCommandBuffers = command_buffers;

	VkSemaphore signal_semaphores[] = {render_finished[current_image]->get_handle()};

	info_submit.signalSemaphoreCount = 1;
	info_submit.pSignalSemaphores = signal_semaphores;
	
	image_retrieved->wait();
	image_retrieved->reset();

	VkResult r = vkQueueSubmit(queue, 1, &info_submit, frame_finished[current_image]->get_handle());
	VERIFY(r, "Failed to submit command buffer to graphics queue.")
	
	return true;
}

void swapchain::refresh_swap_chain()
{
	vkDeviceWaitIdle(get_device()); //Waits for all queues to finish what they're doing...

	destroy_swap_chain();
	create_swap_chain();

	for(size_t i = 0; i < swapchain_resize_callbacks.size(); i++)
		swapchain_resize_callbacks[i](this);

	create_framebuffers(refresh_rp);
}

void swapchain::retrieve_next_image(uint32_t* frame_index, bool* should_retry)
{	
	VkResult r = vkAcquireNextImageKHR(get_device(), vk_swapchain, UINT64_MAX, VK_NULL_HANDLE, image_retrieved->get_handle(), &current_image);
	
	if(r == VK_ERROR_OUT_OF_DATE_KHR)
	{
	#ifdef DEBUG_PRINT_SUCCESS
		std::cout << "[VK|INF] Swap chain out of date. Refreshing..." << std::endl;
	#endif
		refresh_swap_chain();
		*should_retry = true;
		return;
	}
	else if(r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR)
	{
		VERIFY_NORETURN(r, "Failed to retrieve swapchain image.")
	}

	frame_finished[current_image]->wait(); //Waits for all previous frame operations on the GPU to finish.
	frame_finished[current_image]->reset(); //Resets the fence.
	
	*frame_index = current_image;
	*should_retry = false;
}