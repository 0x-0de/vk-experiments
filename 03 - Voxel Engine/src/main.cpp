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

#define GLFW_DEFAULT_WIDTH 1280
#define GLFW_DEFAULT_HEIGHT 720

#define SHADER_MODULES_COUNT 6
#define COMMAND_POOLS_COUNT 1
#define VULKAN_QUEUES_COUNT 2
#define PIPELINES_COUNT 3
#define RENDER_TARGETS_COUNT 3

static VkShaderModule shader_modules[SHADER_MODULES_COUNT];
static VkCommandPool command_pools[COMMAND_POOLS_COUNT];
static VkQueue queues[VULKAN_QUEUES_COUNT];
static pipeline* pipelines[PIPELINES_COUNT];
static alloc::image render_targets[RENDER_TARGETS_COUNT];
static VkImageView render_target_views[RENDER_TARGETS_COUNT];

#define SHADER_VERTEX_MAIN 0
#define SHADER_FRAGMENT_MAIN 1
#define SHADER_VERTEX_WIREFRAME 2
#define SHADER_FRAGMENT_WIREFRAME 3
#define SHADER_VERTEX_SELECTION 4
#define SHADER_FRAGMENT_SELECTION 5

#define PIPELINE_MAIN 0
#define PIPELINE_WIREFRAME 1
#define PIPELINE_SELECTION_DEBUG 2
#define PIPELINE_SELECTION 3

#define QUEUE_GRAPHICS 0
#define QUEUE_PRESENT 1

#define RENDER_TARGET_DEPTH_BUFFER 0
#define RENDER_TARGET_SELECTION_BUFFER 1
#define RENDER_TARGET_SELECTION_DEPTH_BUFFER 2

static camera3d* camera;

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

bool load_graphics_pipelines(swapchain* sc, pipeline_vertex_input pvi, descriptor* desc, render_pass* rp_main, render_pass* rp_selection)
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
		
	if(!pipelines[PIPELINE_MAIN]->build(rp_main)) return false;

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
		
	if(!pipelines[PIPELINE_WIREFRAME]->build(rp_main)) return false;
	
	pipelines[PIPELINE_SELECTION_DEBUG]->add_shader_module(shader_modules[SHADER_VERTEX_SELECTION], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_SELECTION_DEBUG]->add_shader_module(shader_modules[SHADER_FRAGMENT_SELECTION], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	pipelines[PIPELINE_SELECTION_DEBUG]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_SELECTION_DEBUG]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_SELECTION_DEBUG]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_SELECTION_DEBUG]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_SELECTION_DEBUG]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
		
	if(!pipelines[PIPELINE_SELECTION_DEBUG]->build(rp_main)) return false;
	
	/*
	pipelines[PIPELINE_SELECTION]->add_shader_module(shader_modules[SHADER_VERTEX_SELECTION], VK_SHADER_STAGE_VERTEX_BIT, "main");
	pipelines[PIPELINE_SELECTION]->add_shader_module(shader_modules[SHADER_FRAGMENT_SELECTION], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	pipelines[PIPELINE_SELECTION]->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	pipelines[PIPELINE_SELECTION]->add_color_blend_state(get_color_blend_attachment_none());
	pipelines[PIPELINE_SELECTION]->set_pipeline_vertex_input_state(pvi);
	pipelines[PIPELINE_SELECTION]->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	pipelines[PIPELINE_SELECTION]->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
		
	if(!pipelines[PIPELINE_SELECTION]->build(rp_selection)) return false;
	*/
	
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
	if(!alloc::new_image(&render_targets[RENDER_TARGET_SELECTION_BUFFER], sc->get_extent().width, sc->get_extent().height, VK_FORMAT_R32G32B32A32_SFLOAT, ALLOC_USAGE_COLOR_ATTACHMENT)) return false;
	if(!alloc::new_image(&render_targets[RENDER_TARGET_SELECTION_DEPTH_BUFFER], sc->get_extent().width, sc->get_extent().height, depth_format, ALLOC_USAGE_DEPTH_ATTACHMENT)) return false;

	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_DEPTH_BUFFER], render_targets[RENDER_TARGET_DEPTH_BUFFER].vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) return false;
	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_SELECTION_BUFFER], render_targets[RENDER_TARGET_SELECTION_BUFFER].vk_image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT)) return false;
	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_SELECTION_DEPTH_BUFFER], render_targets[RENDER_TARGET_SELECTION_DEPTH_BUFFER].vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) return false;

	return true;
}

void unload_render_targets()
{
	for(size_t i = 0; i < RENDER_TARGETS_COUNT; i++)
	{
		vkDestroyImageView(get_device(), render_target_views[i], nullptr);
		alloc::free(render_targets[i]);
	}
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
}

void swapchain_resize_callback(swapchain* sc)
{
	std::cout << "Recreating depth buffer..." << std::endl;

	VkFormat depth_format;
	if(!utils::find_best_depth_format(&depth_format)) std::cout << "How did we get here?" << std::endl;
	
	vkDestroyImageView(get_device(), render_target_views[RENDER_TARGET_DEPTH_BUFFER], nullptr);
	alloc::free(render_targets[RENDER_TARGET_DEPTH_BUFFER]);

	if(!alloc::new_image(&render_targets[RENDER_TARGET_DEPTH_BUFFER], sc->get_extent().width, sc->get_extent().height, depth_format, ALLOC_USAGE_DEPTH_ATTACHMENT)) throw std::runtime_error("Failed to recreate depth buffer upon swapchain resize.");

	if(!utils::create_image_view(&render_target_views[RENDER_TARGET_DEPTH_BUFFER], render_targets[RENDER_TARGET_DEPTH_BUFFER].vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) throw std::runtime_error("Failed to recreate depth buffer upon swapchain resize.");

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
	
	if(!init_vulkan_application(window)) return 1;
	
	if(!load_command_pools()) return 1;
	get_vulkan_queues();
	
	alloc::init(queues[QUEUE_GRAPHICS], command_pools[0]);

	swapchain* sc = new swapchain(window);
	sc->add_swapchain_resize_callback(swapchain_resize_callback);
	if(!load_render_targets(sc)) return 1;
	sc->add_swapchain_render_target(render_target_views[RENDER_TARGET_DEPTH_BUFFER]);

	render_pass* rp_main = new render_pass();
	//render_pass* rp_selection = new render_pass();

	VkFormat depth_format;
	if(!utils::find_best_depth_format(&depth_format)) std::cout << "How did we get here?" << std::endl;
	
	rp_main->add_attachment(render_pass::create_render_pass_attachment_default_color(sc->get_format().format));
	rp_main->add_attachment(render_pass::create_render_pass_attachment_default_depth(depth_format));
	
	//rp_selection->add_attachment(render_pass::create_render_pass_attachment_default_color(VK_FORMAT_R32G32B32A32_SFLOAT));
	//rp_selection->add_attachment(render_pass::create_render_pass_attachment_default_depth(depth_format));
	
	subpass sp{};
	sp.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sp.color_attachment_indices = {0};
	sp.depth_attachment_index = 1;
	rp_main->add_subpass(sp);
	//rp_selection->add_subpass(sp);
	
	rp_main->build();
	//rp_selection->build();
	
	//VkFramebuffer fb_selection;
	//if(!swapchain::create_framebuffer(&fb_selection, rp_selection, {selection_buffer_view, selection_depth_buffer_view}, sc->get_extent())) return 1;

	uint32_t frame_count = sc->get_image_count();
	
	if(!sc->create_framebuffers(rp_main)) return 1;
	if(!load_shaders()) return 1;
	
	pipeline_vertex_input pvi;
	pvi.vertex_binding = create_vertex_input_binding(0, 8 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	pvi.vertex_attribs.resize(3);
	pvi.vertex_attribs[0] = create_vertex_input_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	pvi.vertex_attribs[1] = create_vertex_input_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
	pvi.vertex_attribs[2] = create_vertex_input_attribute(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float));

	uint32_t img_width, img_height, img_channels;
	char* img_data;
	if(!utils::load_bmp_texture("../res/textures/smile.bmp", &img_width, &img_height, &img_channels, &img_data)) return 1;

	descriptor* desc = new descriptor(frame_count);

	desc->add_descriptor_binding_buffer(48 * sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);

	desc->build();
	
	if(!load_graphics_pipelines(sc, pvi, desc, rp_main, nullptr)) return 1;
	unload_shaders();
	
	uint32_t num_images = sc->get_image_count();
	
	command_buffer** cmd_buffer = new command_buffer*[num_images];
	for(size_t i = 0; i < num_images; i++)
		cmd_buffer[i] = new command_buffer(command_pools[0]);

	camera = new camera3d(math::vec3(128, 160, 128));
	
	sector::init();
	
	sector* sec = new sector(0, 0, 0);
	sec->generate();
	sec->build();
	
	double timer = glfwGetTime();
	uint32_t frames = 0;

	double delta_timer = glfwGetTime();
	double update_time = 0;

	bool window_focused = false;
	
	uint8_t current_pipeline = PIPELINE_MAIN;
	bool pipeline_toggle = false;
	
	//semaphore* sp_selection_buffer = new semaphore();
	//fence* fnc_selection_buffer = new fence();

	while(!glfwWindowShouldClose(window))
	{
		double delta = glfwGetTime() - delta_timer;
		delta_timer = glfwGetTime();

		update_time += delta;

		if(glfwGetTime() - timer >= 1.0)
		{
			std::cout << "[VK|INF] FPS: " << frames << std::endl;
			frames = 0;
			timer = glfwGetTime();
		}
		glfwPollEvents();

		if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			window_focused = true;
		}
		else if(glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			window_focused = false;
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
		
		/*
		cmd_buffer[frame_index]->reset();
		cmd_buffer[frame_index]->begin_recording();
		cmd_buffer[frame_index]->begin_render_pass(rp_selection, fb_selection, sc->get_extent());
		cmd_buffer[frame_index]->bind_pipeline(pipelines[PIPELINE_SELECTION]);
		cmd_buffer[frame_index]->set_viewport(sc->get_viewport(), sc->get_scissor());
		cmd_buffer[frame_index]->bind_descriptor_set(pipelines[PIPELINE_SELECTION]->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
		sec->draw(cmd_buffer[frame_index], desc, frame_index);
		cmd_buffer[frame_index]->end_render_pass();
		cmd_buffer[frame_index]->end_recording();
		
		VkSubmitInfo info_submit_sb{};
		info_submit_sb.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info_submit_sb.waitSemaphoreCount = 0;
		info_submit_sb.signalSemaphoreCount = 0;
		VkCommandBuffer command_buffers[] = {cmd_buffer[frame_index]->get_handle()};
		info_submit_sb.commandBufferCount = 1;
		info_submit_sb.pCommandBuffers = command_buffers;
		
		VkResult r = vkQueueSubmit(queues[QUEUE_GRAPHICS], 1, &info_submit_sb, fnc_selection_buffer->get_handle());
		VERIFY_NORETURN(r, "Failed to submit command buffer to graphics queue.")
		
		fnc_selection_buffer->wait();
		fnc_selection_buffer->reset();
		
		//float selection_data[4];
		//utils::read_pixel(&selection_buffer, 0, 0, selection_data);
		
		//std::cout << selection_data[0] << ", " << selection_data[1] << ", " << selection_data[2] << ", " << selection_data[3] << std::endl;
		*/

		cmd_buffer[frame_index]->reset();
		cmd_buffer[frame_index]->begin_recording();
		cmd_buffer[frame_index]->begin_render_pass(rp_main, sc->get_framebuffer(frame_index), sc->get_extent());
		cmd_buffer[frame_index]->bind_pipeline(pipelines[current_pipeline]);
		cmd_buffer[frame_index]->set_viewport(sc->get_viewport(), sc->get_scissor());
		cmd_buffer[frame_index]->bind_descriptor_set(pipelines[0]->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
		sec->draw(cmd_buffer[frame_index], desc, frame_index);
		cmd_buffer[frame_index]->end_render_pass();
		cmd_buffer[frame_index]->end_recording();

		camera->freemove(window, delta * 2);
		camera->update_rot(window, 1, window_focused);
		
		if(!sc->image_render(queues[QUEUE_GRAPHICS], cmd_buffer[frame_index])) return 1;
		sc->image_present(queues[QUEUE_PRESENT]);

		frames++;
	}
	
	vkDeviceWaitIdle(get_device());
	
	//delete sp_selection_buffer;
	
	for(size_t i = 0; i < num_images; i++)
	{
		delete cmd_buffer[i];
	}

	delete sec;
	delete camera;
	delete[] cmd_buffer;
	unload_graphics_pipelines();
	delete[] img_data;
	delete desc;
	unload_command_pools();
	//delete rp_selection;
	delete rp_main;
	unload_render_targets();
	delete sc;
	alloc::deinit();
	deinit_vulkan_application();
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}