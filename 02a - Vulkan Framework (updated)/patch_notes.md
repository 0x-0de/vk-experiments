# 0de's Vulkan Framework

## Pre 0.5
---
**v0.1** - This version is retroactively used to mark the simplest triangle demo used in experiment 1, with the basic abstraction found in 1a. For the current codebase, this version of the framework was completed in mid-July 2025.

**v0.2** - This version is retroactively used to mark the introduction of vertex and index buffers to the framework, each with singular memory allocations. This version of the framework was completed two days after v0.1.

**v0.3** - This version is retroactively used to mark the introduction of a custom memory allocator to the framework, and the replacement of existing VkBuffer objects with alloc::buffer objects pointing to automatically-suballocated VkDeviceMemory. This version was completed about a week after v0.2, in late July 2025.

**v0.4** - This version is retroactively used to mark the introduction of descriptor sets to the framework, as well as resolving existing issues with the swap chain (these fixes are present in experiment 1b). This version was completed in mid-August 2025.

**v0.5** - This version was the first publicly released version of the framework, which corresponds with the introduction of textures and image allocations.

## v0.5 - August 21, 2025
- Initial public & numbered version.
- Framework automatically sets up a Vulkan instance, selects a physical device, and creates a logical device (vksetup.cpp).
	- Currently all extensions, features, and aspects of device selection/creation are hard-coded.
- Implemented objects representing common Vulkan structures, such as...
	- Pipelines (pipeline.cpp).
	- Render passes (renderpass.cpp), as a single hard-coded structure.
	- Command buffers (cmdbuffer.cpp).
	- Swap chains (through the Vulkan swapchain extension, swapchain.cpp).
	- Descriptor sets (descriptor.cpp).
	- Simple textures (texture.cpp).
	- Synchronization objects (vksync.cpp).
- Pipelines use custom vertex input structures, which aid in automatically generating a vertex input state, as well as shader structures for assisting in shader module generation, and "views", which contain information about the viewport and scissor extants that a pipeline will use.
- Optional macros display information about Vulkan object creation and destruction, as well as some properties of created objects.
- Implemented a custom memory allocator to assist in allocating memory for vertex buffers, index buffers, and texture images.
- Implemented a custom linear algebra library to assist in elementary rendering techniques such as the usage of transformation matrices.
- Project contains a simple rotating textured quad.

## v0.6 - August 30, 2025
- Added perspective projection and camera look-at matrices to the linear algebra library.
- Added pseudo-random number generation, linear and cosine interpolation, alongside basic gradient noise to the linear algebra library.
- Allocating images now must specify a usage for image allocation, rather than assuming every allocation will be as a staged 2D sampler.
	- Available image allocation types are ALLOC_USAGE_TEXTURE, ALLOC_USAGE_COLOR_ATTACHMENT, ALLOC_USAGE_COLOR_ATTACHMENT_CPU_VISIBLE and ALLOC_USAGE_DEPTH_ATTACHMENT.
- Added mesh and 3D camera objects to the framework, as utilities.
- Project now contains two quads within a mesh, which are being drawn in 3D and depth-tested.
- Render passes are now properly programmable.
	- Custom attachment and subpass structures have been made to help specify different aspects of a render pass.
- Swapchains can now have additional render targets (such as depth buffers), and can also have additional callbacks for when the swapchain is recreated.