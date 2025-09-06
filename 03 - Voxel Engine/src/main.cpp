#include <iostream>

#include "renderer/cmdbuffer.h"
#include "renderer/descriptor.h"
#include "renderer/pipeline.h"
#include "renderer/renderpass.h"
#include "renderer/swapchain.h"
#include "renderer/texture.h"
#include "renderer/vksetup.h"

#include "utils/alloc.h"
#include "utils/camera.h"
#include "utils/image_utils.h"
#include "utils/linalg.h"
#include "utils/vksync.h"

#include "voxel/sector.h"

#include "../lib/GLFW/glfw3.h"

#include <cmath>

#define INFO_LOG(x) std::cout << "[VOX|INF] " << x << std::endl

#define GLFW_DEFAULT_WIDTH 1280
#define GLFW_DEFAULT_HEIGHT 720

#define SHADER_MODULES_COUNT 6
#define COMMAND_POOLS_COUNT 1
#define VULKAN_QUEUES_COUNT 2
#define RENDER_PASS_COUNT 2
#define PIPELINES_COUNT 4
#define RENDER_TARGETS_COUNT 3
#define FRAMEBUFFERS_COUNT 1

static VkShaderModule shader_modules[SHADER_MODULES_COUNT];
static VkCommandPool command_pools[COMMAND_POOLS_COUNT];
static VkQueue queues[VULKAN_QUEUES_COUNT];
static render_pass* render_passes[RENDER_PASS_COUNT];
static pipeline* pipelines[PIPELINES_COUNT];
static alloc::image render_targets[RENDER_TARGETS_COUNT];
static VkImageView render_target_views[RENDER_TARGETS_COUNT];
static VkFramebuffer target_framebuffers[FRAMEBUFFERS_COUNT];

#define SHADER_VERTEX_MAIN 0
#define SHADER_FRAGMENT_MAIN 1
#define SHADER_VERTEX_WIREFRAME 2
#define SHADER_FRAGMENT_WIREFRAME 3
#define SHADER_VERTEX_SELECTION 4
#define SHADER_FRAGMENT_SELECTION 5

#define RENDER_PASS_MAIN 0
#define RENDER_PASS_SELECTION 1

#define PIPELINE_MAIN 0
#define PIPELINE_WIREFRAME 1
#define PIPELINE_SELECTION_DEBUG 2
#define PIPELINE_SELECTION 3

#define QUEUE_GRAPHICS 0
#define QUEUE_PRESENT 1

#define RENDER_TARGET_DEPTH_BUFFER 0
#define RENDER_TARGET_SELECTION_BUFFER 1
#define RENDER_TARGET_SELECTION_DEPTH_BUFFER 2

#define FRAMEBUFFER_SELECTION 0

static camera3d* camera;

static uint32_t voxel_selection_data[4];
static bool window_resized = false;

#define SELECTION_BUFFER_FORMAT VK_FORMAT_R32G32B32A32_UINT

static alloc::buffer selection_image_read;

void window_resize_callback(GLFWwindow* window, int width, int height)
{
	window_resized = true;
}

bool load_shaders()
{	
	if(!create_shader_module(&shader_modules[SHADER_VERTEX_MAIN], "../res/shaders/main_vert.spv")) return false;
	if(!create_shader_module(&shader_modules[SHADER_FRAGMENT_MAIN], "../res/shaders/main_frag.spv")) return false;
	
	if(!create_shader_module(&shader_modules[SHADER_VERTEX_WIREFRAME], "../res/shaders/wireframe_vert.spv")) return false;
	if(!create_shader_module(&shader_modules[SHADER_FRAGMENT_WIREFRAME], "../res/shaders/wireframe_frag.spv")) return false;
	
	if(!create_shader_module(&shader_modules[SHADER_VERTEX_SELECTION], "../res/shaders/selection_vert.spv")) return false;
	if(!create_shader_module(&shader_modules[SHADER_FRAGMENT_SELECTION], "../res/shaders/selection_frag.spv")) return false;
	
	return true;
}

void unload_shaders()
{
	for(size_t i = 0; i < SHADER_MODULES_COUNT; i++)
		vkDestroyShaderModule(get_device(), shader_modules[i], nullptr);
}

bool load_render_passes(swapchain* sc)
{
	for(size_t i = 0; i < RENDER_PASS_COUNT; i++)
		render_passes[i] = new render_pass();

	VkFormat depth_format;
	if(!utils::find_best_depth_format(&depth_format)) std::cout << "How did we get here?" << std::endl;
	
	render_passes[RENDER_PASS_MAIN]->add_attachment(render_pass::create_render_pass_attachment_default_color(sc->get_format().format));
	render_passes[RENDER_PASS_MAIN]->add_attachment(render_pass::create_render_pass_attachment_default_depth(depth_format));
	
	render_pass_attachment rpa_selection_color = render_pass::create_render_pass_attachment_default_color(SELECTION_BUFFER_FORMAT);
	rpa_selection_color.final_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	
	render_passes[RENDER_PASS_SELECTION]->add_attachment(rpa_selection_color);
	render_passes[RENDER_PASS_SELECTION]->add_attachment(render_pass::create_render_pass_attachment_default_depth(depth_format));
	
	subpass sp{};
	sp.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sp.color_attachment_indices = {0};
	sp.depth_attachment_index = 1;
	
	render_passes[RENDER_PASS_MAIN]->add_subpass(sp);
	render_passes[RENDER_PASS_SELECTION]->add_subpass(sp);
	
	if(!render_passes[RENDER_PASS_MAIN]->build()) return false;
	if(!render_passes[RENDER_PASS_SELECTION]->build()) return false;
	
	return true;
}

void unload_render_passes()
{
	for(size_t i = 0; i < RENDER_PASS_COUNT; i++)
		delete render_passes[i];
}

bool load_graphics_pipelines(swapchain* sc, pipeline_vertex_input pvi, descriptor* desc)
{
	for(size_t i = 0; i < PIPELINES_COUNT; i++)
		pipelines[i] = new pipeline();
	
	pipelines[PIPELINE_MAIN]->add_shader_module(shader_modules[SHADER_VERTEX_MAIN], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_MAIN]->add_shader_module(shader_modules[SHADER_FRAGMENT_MAIN], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	pipelines[PIPELINE_MAIN]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_MAIN]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_MAIN]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_MAIN]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_MAIN]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
		
	if(!pipelines[PIPELINE_MAIN]->build(render_passes[RENDER_PASS_MAIN])) return false;

	pipelines[PIPELINE_WIREFRAME]->add_shader_module(shader_modules[SHADER_VERTEX_WIREFRAME], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_WIREFRAME]->add_shader_module(shader_modules[SHADER_FRAGMENT_WIREFRAME], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	pipelines[PIPELINE_WIREFRAME]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_WIREFRAME]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_WIREFRAME]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_WIREFRAME]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_WIREFRAME]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
	
	VkPipelineRasterizationStateCreateInfo info_raster = getdefault_rasterization_state();
	info_raster.polygonMode = VK_POLYGON_MODE_LINE;
	info_raster.lineWidth = 3;
	
	pipelines[PIPELINE_WIREFRAME]->set_pipeline_rasterization_state(info_raster);
		
	if(!pipelines[PIPELINE_WIREFRAME]->build(render_passes[RENDER_PASS_MAIN])) return false;
	
	pipelines[PIPELINE_SELECTION_DEBUG]->add_shader_module(shader_modules[SHADER_VERTEX_SELECTION], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_SELECTION_DEBUG]->add_shader_module(shader_modules[SHADER_FRAGMENT_SELECTION], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	pipelines[PIPELINE_SELECTION_DEBUG]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_SELECTION_DEBUG]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_SELECTION_DEBUG]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_SELECTION_DEBUG]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_SELECTION_DEBUG]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
		
	if(!pipelines[PIPELINE_SELECTION_DEBUG]->build(render_passes[RENDER_PASS_MAIN])) return false;
		
	pipelines[PIPELINE_SELECTION]->add_shader_module(shader_modules[SHADER_VERTEX_SELECTION], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_SELECTION]->add_shader_module(shader_modules[SHADER_FRAGMENT_SELECTION], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
		
	pipelines[PIPELINE_SELECTION]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_SELECTION]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_SELECTION]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_SELECTION]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_SELECTION]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
				
	if(!pipelines[PIPELINE_SELECTION]->build(render_passes[RENDER_PASS_SELECTION])) return false;
		
	return true;
}

void unload_graphics_pipelines()
{
	for(size_t i = 0; i < PIPELINES_COUNT; i++)
		delete pipelines[i];
}

bool load_command_pools()
{
	queue_family qf = find_physical_device_queue_families(get_selected_physical_device());
		
	if(!create_command_pool(&command_pools[0], qf.queue_index_graphics.value())) return false;
	
	return true;
}

void unload_command_pools()
{
	for(size_t i = 0; i < COMMAND_POOLS_COUNT; i++)
	{
		vkDestroyCommandPool(get_device(), command_pools[i], nullptr);
	}
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Deinitialized all command pools." << std::endl;
#endif
}

void get_vulkan_queues()
{
	queue_family qf = find_physical_device_queue_families(get_selected_physical_device());
		
	vkGetDeviceQueue(get_device(), qf.queue_index_graphics.value(), 0, &queues[QUEUE_GRAPHICS]);
	vkGetDeviceQueue(get_device(), qf.queue_index_present.value(), 0, &queues[QUEUE_PRESENT]);
}

bool load_render_targets(swapchain* sc)
{
	VkFormat depth_format;
	if(!utils::find_best_depth_format(&depth_format)) return false;

	if(!alloc::new_image(&render_targets[RENDER_TARGET_DEPTH_BUFFER], sc->get_extent().width, sc->get_extent().height, depth_format, ALLOC_USAGE_DEPTH_ATTACHMENT)) return false;
	if(!alloc::new_image(&render_targets[RENDER_TARGET_SELECTION_BUFFER], sc->get_extent().width, sc->get_extent().height, SELECTION_BUFFER_FORMAT, ALLOC_USAGE_COLOR_ATTACHMENT)) return false;
	if(!alloc::new_image(&render_targets[RENDER_TARGET_SELECTION_DEPTH_BUFFER], sc->get_extent().width, sc->get_extent().height, depth_format, ALLOC_USAGE_DEPTH_ATTACHMENT)) return false;

	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_DEPTH_BUFFER], render_targets[RENDER_TARGET_DEPTH_BUFFER].vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) return false;
	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_SELECTION_BUFFER], render_targets[RENDER_TARGET_SELECTION_BUFFER].vk_image, SELECTION_BUFFER_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT)) return false;
	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_SELECTION_DEPTH_BUFFER], render_targets[RENDER_TARGET_SELECTION_DEPTH_BUFFER].vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) return false;
	
	uint32_t pixel_size = sc->get_extent().width * sc->get_extent().height * 4 * sizeof(uint32_t);
	if(!alloc::new_buffer(&selection_image_read, pixel_size, ALLOC_USAGE_GENERIC_BUFFER_CPU_VISIBLE)) return 1;

	return true;
}

void unload_render_targets()
{
	for(size_t i = 0; i < RENDER_TARGETS_COUNT; i++)
	{
		vkDestroyImageView(get_device(), render_target_views[i], nullptr);
		alloc::free(render_targets[i]);
	}
	
	alloc::free(selection_image_read);
}

void update_uniforms(uint16_t descriptor_set, double time, descriptor* desc, float width, float height)
{
	math::mat projection = math::perspective(70.0 * 3.1415926 / 180.0, width / height, 0.01, 1000.0);
	float projection_data[16];
	
	math::mat rotation = math::rotation(math::vec3(0, 0, time));
	math::mat transform = math::transform(math::vec3(std::sin(time / 4) / 2, 0, -5), rotation, math::vec3(1, 1, 1));
	float transform_data[16];

	math::mat look_at = camera->view_matrix();
	float view_data[16];

	transform.get_data(transform_data);
	projection.get_data(projection_data);
	look_at.get_data(view_data);

	desc->place_data(descriptor_set, 0, 0, 16 * sizeof(float), transform_data);
	desc->place_data(descriptor_set, 0, 16 * sizeof(float), 16 * sizeof(float), projection_data);
	desc->place_data(descriptor_set, 0, 32 * sizeof(float), 16 * sizeof(float), view_data);
	desc->place_data(descriptor_set, 0, 48 * sizeof(float), 4 * sizeof(uint32_t), voxel_selection_data);
}

void swapchain_resize_callback(swapchain* sc)
{
	vkDestroyFramebuffer(get_device(), target_framebuffers[FRAMEBUFFER_SELECTION], nullptr);
	unload_render_targets();
	
	load_render_targets(sc);
	
	if(!swapchain::create_framebuffer(&target_framebuffers[FRAMEBUFFER_SELECTION], render_passes[RENDER_PASS_SELECTION], {render_target_views[RENDER_TARGET_SELECTION_BUFFER], render_target_views[RENDER_TARGET_SELECTION_DEPTH_BUFFER]}, sc->get_extent())) std::cout << "How tf did we get here?" << std::endl;

	sc->clear_swapchain_render_targets();
	sc->add_swapchain_render_target(render_target_views[RENDER_TARGET_DEPTH_BUFFER]);
}

int main()
{
	glfwInit();
	
	//GLFW assumes you're using an OpenGL context. This hint disables said assumption.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(GLFW_DEFAULT_WIDTH, GLFW_DEFAULT_HEIGHT, "Voxel Engine", NULL, NULL);
	glfwSetFramebufferSizeCallback(window, window_resize_callback);
	
	INFO_LOG("Initializing Vulkan.");
	if(!init_vulkan_application(window)) return 1;
	
	INFO_LOG("Loading Vulkan command pools and queues.");
	if(!load_command_pools()) return 1;
	get_vulkan_queues();
	
	INFO_LOG("Initializing memory allocator.");
	alloc::init(queues[QUEUE_GRAPHICS], command_pools[0]);

	INFO_LOG("Initializing Vulkan swapchain and render targets.");
	swapchain* sc = new swapchain(window);
	sc->add_swapchain_resize_callback(swapchain_resize_callback);
	if(!load_render_targets(sc)) return 1;
	sc->add_swapchain_render_target(render_target_views[RENDER_TARGET_DEPTH_BUFFER]);

	INFO_LOG("Creating necessary render passes.");
	if(!load_render_passes(sc)) return 1;
	
	INFO_LOG("Creating Vulkan framebuffers.");
	if(!swapchain::create_framebuffer(&target_framebuffers[FRAMEBUFFER_SELECTION], render_passes[RENDER_PASS_SELECTION], {render_target_views[RENDER_TARGET_SELECTION_BUFFER], render_target_views[RENDER_TARGET_SELECTION_DEPTH_BUFFER]}, sc->get_extent())) return 1;

	uint32_t frame_count = sc->get_image_count();
	
	if(!sc->create_framebuffers(render_passes[RENDER_PASS_MAIN])) return 1;
	
	INFO_LOG("Loading shaders.");
	if(!load_shaders()) return 1;
	
	pipeline_vertex_input pvi;
	pvi.vertex_binding = create_vertex_input_binding(0, 8 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	pvi.vertex_attribs.resize(3);
	pvi.vertex_attribs[0] = create_vertex_input_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	pvi.vertex_attribs[1] = create_vertex_input_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
	pvi.vertex_attribs[2] = create_vertex_input_attribute(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float));

	INFO_LOG("Loading descriptor sets.");
	descriptor* desc = new descriptor(frame_count);

	desc->add_descriptor_binding_buffer(52 * sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);

	desc->build();
	
	INFO_LOG("Loading Vulkan graphics pipelines.");
	if(!load_graphics_pipelines(sc, pvi, desc)) return 1;
	unload_shaders();
	
	uint32_t num_images = sc->get_image_count();
	
	INFO_LOG("Creating command buffers.");
	command_buffer** cmd_buffer = new command_buffer*[num_images];
	for(size_t i = 0; i < num_images; i++)
		cmd_buffer[i] = new command_buffer(command_pools[0]);

	INFO_LOG("Creating 3D camera.");
	camera = new camera3d(math::vec3(32, 40, 32));
	
	sector::init(glfwGetTime() * 1000000000);
	
	sector* sec = new sector(0, 0, 0);
	INFO_LOG("Generating sector.");
	sec->generate();
	INFO_LOG("Loading sector.");
	sec->build();
	
	double timer = glfwGetTime();
	uint32_t frames = 0;

	double delta_timer = glfwGetTime();
	double update_time = 0;

	bool window_focused = false;
	bool should_rotate_camera = false;
	
	uint8_t current_pipeline = PIPELINE_MAIN;
	bool pipeline_toggle = false;
	
	//semaphore* sp_selection_buffer = new semaphore();
	fence* fnc_selection_buffer = new fence();
	
	int voxel_timer = 0;
	
	while(!glfwWindowShouldClose(window))
	{
		double delta = glfwGetTime() - delta_timer;
		delta_timer = glfwGetTime();

		update_time += delta;
		
		int window_width, window_height;
		glfwGetFramebufferSize(window, &window_width, &window_height);
		
		if(window_resized)
		{
			if(sc->get_extent().width != window_width || sc->get_extent().height != window_height)
				sc->refresh_swap_chain();
			window_resized = false;
		}

		if(glfwGetTime() - timer >= 1.0)
		{
			std::cout << "[VK|INF] FPS: " << frames << std::endl;
			frames = 0;
			timer = glfwGetTime();
		}
		glfwPollEvents();
		
		double cursor_x_raw, cursor_y_raw;
		glfwGetCursorPos(window, &cursor_x_raw, &cursor_y_raw);
		
		int cursor_x = (int) cursor_x_raw;
		int cursor_y = (int) cursor_y_raw;

		if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
		{
			window_focused = true;
		}
		else if(glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			
			window_focused = false;
		}
		
		if(window_focused)
		{
			if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				should_rotate_camera = false;
			}
			else
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				should_rotate_camera = true;
			}
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			should_rotate_camera = false;
		}
		
		if(glfwGetKey(window, GLFW_KEY_F) && !pipeline_toggle)
		{
			current_pipeline = PIPELINE_MAIN;
			pipeline_toggle = true;
		}
		else if(glfwGetKey(window, GLFW_KEY_G) && !pipeline_toggle)
		{
			current_pipeline = PIPELINE_WIREFRAME;
			pipeline_toggle = true;
		}
		else if(glfwGetKey(window, GLFW_KEY_H) && !pipeline_toggle)
		{
			current_pipeline = PIPELINE_SELECTION_DEBUG;
			pipeline_toggle = true;
		}
		else if(!glfwGetKey(window, GLFW_KEY_F) && !glfwGetKey(window, GLFW_KEY_G) && !glfwGetKey(window, GLFW_KEY_H))
		{
			pipeline_toggle = false;
		}
		
		uint32_t frame_index;

		bool should_retry = true;
		while(should_retry) sc->retrieve_next_image(&frame_index, &should_retry);

		update_uniforms(frame_index, update_time, desc, sc->get_viewport().width, sc->get_viewport().height);
		
		VkCommandBuffer command_buffers[] = {cmd_buffer[frame_index]->get_handle()};
		
		VkSubmitInfo info_submit_generic{};
		info_submit_generic.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info_submit_generic.commandBufferCount = 1;
		info_submit_generic.pCommandBuffers = command_buffers;
		
		cmd_buffer[frame_index]->reset();
		cmd_buffer[frame_index]->begin_recording();
		cmd_buffer[frame_index]->begin_render_pass(render_passes[RENDER_PASS_SELECTION], target_framebuffers[FRAMEBUFFER_SELECTION], sc->get_extent());
		cmd_buffer[frame_index]->bind_pipeline(pipelines[PIPELINE_SELECTION]);
		cmd_buffer[frame_index]->set_viewport(sc->get_viewport(), sc->get_scissor());
		cmd_buffer[frame_index]->bind_descriptor_set(pipelines[PIPELINE_SELECTION]->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
		sec->draw(cmd_buffer[frame_index], desc, frame_index);
		cmd_buffer[frame_index]->end_render_pass();
		cmd_buffer[frame_index]->end_recording();
		
		VkResult r = vkQueueSubmit(queues[QUEUE_GRAPHICS], 1, &info_submit_generic, fnc_selection_buffer->get_handle());
		VERIFY_NORETURN(r, "Failed to submit command buffer to graphics queue.")
		
		fnc_selection_buffer->wait();
		fnc_selection_buffer->reset();
		
		if(!alloc::copy_image_to_buffer(&render_targets[RENDER_TARGET_SELECTION_BUFFER], &selection_image_read, sc->get_extent().width, sc->get_extent().height, VK_IMAGE_ASPECT_COLOR_BIT)) return 1;
		
		int selection_x = should_rotate_camera ? sc->get_extent().width / 2 : cursor_x;
		int selection_y = should_rotate_camera ? sc->get_extent().height / 2 : cursor_y;
		
		int selection_offset = (selection_y * sc->get_extent().width + selection_x) * 4 * sizeof(float);
				
		if(selection_x >= sc->get_extent().width || selection_y >= sc->get_extent().height || selection_x < 0 || selection_y < 0)
		{
			voxel_selection_data[3] = 0;
		}
		else alloc::map_data_from_buffer(voxel_selection_data, &selection_image_read, selection_offset, 4 * sizeof(float));
		
		cmd_buffer[frame_index]->reset();
		cmd_buffer[frame_index]->begin_recording();
		cmd_buffer[frame_index]->begin_render_pass(render_passes[RENDER_PASS_MAIN], sc->get_framebuffer(frame_index), sc->get_extent());
		cmd_buffer[frame_index]->bind_pipeline(pipelines[current_pipeline]);
		cmd_buffer[frame_index]->set_viewport(sc->get_viewport(), sc->get_scissor());
		cmd_buffer[frame_index]->bind_descriptor_set(pipelines[0]->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
		sec->draw(cmd_buffer[frame_index], desc, frame_index);
		cmd_buffer[frame_index]->end_render_pass();
		cmd_buffer[frame_index]->end_recording();

		camera->freemove(window, delta * 2);
		camera->update_rot(window, 1, should_rotate_camera);
		
		if(!sc->image_render(queues[QUEUE_GRAPHICS], cmd_buffer[frame_index])) return 1;
		sc->image_present(queues[QUEUE_PRESENT]);
		
		int voxel_x = ((int) voxel_selection_data[0]) >> 16;
		int voxel_y = (((int) voxel_selection_data[0]) >> 8) & 255;
		int voxel_z = ((int) voxel_selection_data[0]) & 255;
		
		int face = (int) voxel_selection_data[1];
		
		std::cout << voxel_x << ", " << voxel_y << ", " << voxel_z << " : " << face << std::endl;
		
		if(voxel_timer > 0)
			voxel_timer--;
		
		if(window_focused && voxel_timer == 0)
		{
			if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
			{
				switch(face)
				{
					case 0:
					case 2:
					case 4:
						break;
					case 1:
						voxel_x -= 1;
						break;
					case 3:
						voxel_y -= 1;
						break;
					case 5:
						voxel_z -= 1;
						break;
				}
				
				sec->set(voxel_x, voxel_y, voxel_z, 0, true);
			}
			
			if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
			{
				switch(face)
				{
					case 1:
					case 3:
					case 5:
						break;
					case 0:
						voxel_x -= 1;
						break;
					case 2:
						voxel_y -= 1;
						break;
					case 4:
						voxel_z -= 1;
						break;
				}
				
				sec->set(voxel_x, voxel_y, voxel_z, 1, true);
			}
			
			voxel_timer = 25;
		}

		frames++;
	}
	
	vkDeviceWaitIdle(get_device());
	
	INFO_LOG("Unloading resources.");
	delete fnc_selection_buffer;
		
	for(size_t i = 0; i < num_images; i++)
	{
		delete cmd_buffer[i];
	}

	delete sec;
	delete camera;
	delete[] cmd_buffer;
	unload_graphics_pipelines();
	delete desc;
	unload_command_pools();
	vkDestroyFramebuffer(get_device(), target_framebuffers[FRAMEBUFFER_SELECTION], nullptr);
	unload_render_passes();
	unload_render_targets();
	delete sc;
	alloc::deinit();
	deinit_vulkan_application();
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}