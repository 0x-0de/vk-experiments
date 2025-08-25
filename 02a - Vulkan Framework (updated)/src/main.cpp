#include <iostream>

#include "mesh/mesh.h"

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

#include "../lib/GLFW/glfw3.h"

#include <cmath>

#define GLFW_DEFAULT_WIDTH 1280
#define GLFW_DEFAULT_HEIGHT 720

#define SHADER_MODULES_COUNT 2
#define COMMAND_POOLS_COUNT 1
#define VULKAN_QUEUES_COUNT 2

#define QUEUE_GRAPHICS 0
#define QUEUE_PRESENT 1

static camera3d* camera;

bool load_shaders(std::vector<VkShaderModule>* modules)
{
	modules->resize(SHADER_MODULES_COUNT);
	
	if(!create_shader_module(&(*modules)[0], "../res/shaders/triangle_vert.spv")) return false;
	if(!create_shader_module(&(*modules)[1], "../res/shaders/triangle_frag.spv")) return false;
	
	return true;
}

bool load_command_pools(std::vector<VkCommandPool>* pools)
{
	queue_family qf = find_physical_device_queue_families(get_selected_physical_device());
	
	pools->resize(COMMAND_POOLS_COUNT);
	
	if(!create_command_pool(pools->data(), qf.queue_index_graphics.value())) return false;
	
	return true;
}

void unload_command_pools(std::vector<VkCommandPool> pools)
{
	for(size_t i = 0; i < pools.size(); i++)
	{
		vkDestroyCommandPool(get_device(), pools[i], nullptr);
	}
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Deinitialized all command pools." << std::endl;
#endif
}

void get_vulkan_queues(std::vector<VkQueue>* queues)
{
	queue_family qf = find_physical_device_queue_families(get_selected_physical_device());
	
	queues->resize(VULKAN_QUEUES_COUNT);
	
	vkGetDeviceQueue(get_device(), qf.queue_index_graphics.value(), 0, queues->data());
	vkGetDeviceQueue(get_device(), qf.queue_index_present.value(), 0, queues->data() + 1);
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

	desc->place_data(descriptor_set, 0, 0, 32 * sizeof(float), transform_data);
	desc->place_data(descriptor_set, 0, 16 * sizeof(float), 16 * sizeof(float), projection_data);
	desc->place_data(descriptor_set, 0, 32 * sizeof(float), 16 * sizeof(float), view_data);
}

int main()
{
	glfwInit();
	
	//GLFW assumes you're using an OpenGL context. This hint disables said assumption.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(GLFW_DEFAULT_WIDTH, GLFW_DEFAULT_HEIGHT, "GLFW Window", NULL, NULL);
	
	if(!init_vulkan_application(window)) return 1;
	
	std::vector<VkCommandPool> command_pools;
	if(!load_command_pools(&command_pools)) return 1;

	std::vector<VkQueue> queues;
	get_vulkan_queues(&queues);
	
	alloc::init(queues[QUEUE_GRAPHICS], command_pools[0]);

	swapchain* sc = new swapchain(window);

	VkFormat depth_format;
	if(!utils::find_best_depth_format(&depth_format)) return 1;

	alloc::image depth_buffer;
	alloc::new_image(&depth_buffer, sc->get_extent().width, sc->get_extent().height, depth_format, ALLOC_USAGE_DEPTH_ATTACHMENT);

	VkImageView depth_buffer_view;
	if(!utils::create_image_view(&depth_buffer_view, depth_buffer.vk_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT)) return 1;

	render_pass* rp = new render_pass(sc->get_format(), depth_format);

	uint32_t frame_count = sc->get_image_count();
	
	if(!sc->create_framebuffers(rp, depth_buffer_view)) return 1;
	
	std::vector<VkShaderModule> shader_modules;
	if(!load_shaders(&shader_modules)) return 1;
	
	pipeline_vertex_input pvi;
	pvi.vertex_binding = create_vertex_input_binding(0, 8 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	pvi.vertex_attribs.resize(3);
	pvi.vertex_attribs[0] = create_vertex_input_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	pvi.vertex_attribs[1] = create_vertex_input_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
	pvi.vertex_attribs[2] = create_vertex_input_attribute(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float));

	uint32_t img_width, img_height, img_channels;
	char* img_data;
	if(!utils::load_bmp_texture("../res/textures/smile.bmp", &img_width, &img_height, &img_channels, &img_data)) return 1;

	texture* tex = new texture(img_data, img_width, img_height, VK_FORMAT_B8G8R8A8_SRGB);

	descriptor* desc = new descriptor(frame_count);

	desc->add_descriptor_binding_buffer(48 * sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);
	desc->add_descriptor_binding_sampler(tex, VK_SHADER_STAGE_FRAGMENT_BIT);

	desc->build();
	
	pipeline* p = new pipeline();
	p->add_shader_module(shader_modules[0], VK_SHADER_STAGE_VERTEX_BIT, "main");
	p->add_shader_module(shader_modules[1], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	p->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	p->add_color_blend_state(get_color_blend_attachment_none());
	p->set_pipeline_vertex_input_state(pvi);
	p->add_descriptor_set_layout(desc->get_descriptor_set_layout());
	p->set_pipeline_depth_stencil_state(create_simple_depth_test_state());
		
	if(!p->build(rp)) return 1;
	
	uint32_t num_images = sc->get_image_count();
	
	command_buffer** cmd_buffer = new command_buffer*[num_images];
	for(size_t i = 0; i < num_images; i++)
		cmd_buffer[i] = new command_buffer(command_pools[0]);
	
	mesh* m = new mesh(pvi);
	m->add_vertex({-0.5, -0.5,  0.5,   1, 1, 1,   0, 0});
	m->add_vertex({ 0.5, -0.5,  0.5,   1, 1, 0,   1, 0});
	m->add_vertex({ 0.5,  0.5,  0.5,   1, 0, 0,   1, 1});
	m->add_vertex({-0.5,  0.5,  0.5,   0, 1, 0,   0, 1});

	m->add_vertex({-0.5, -0.5,  -0.5,   1, 1, 1,   0, 0});
	m->add_vertex({ 0.5, -0.5,  -0.5,   0, 1, 1,   1, 0});
	m->add_vertex({ 0.5,  0.5,  -0.5,   0, 1, 0,   1, 1});
	m->add_vertex({-0.5,  0.5,  -0.5,   0, 0, 1,   0, 1});

	m->add_indices({0, 1, 2, 0, 2, 3});
	m->add_indices({4, 5, 6, 4, 6, 7});

	m->build();

	camera = new camera3d(math::vec3(0, 0, -3));
	
	double timer = glfwGetTime();
	uint32_t frames = 0;

	double delta_timer = glfwGetTime();
	double update_time = 0;

	bool window_focused = false;

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
		
		uint32_t frame_index;

		bool should_retry = true;
		while(should_retry) sc->retrieve_next_image(&frame_index, &should_retry);

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

		update_uniforms(frame_index, update_time, desc, sc->get_viewport().width, sc->get_viewport().height);

		cmd_buffer[frame_index]->reset();

		cmd_buffer[frame_index]->begin_recording();
		cmd_buffer[frame_index]->begin_render_pass(rp, sc->get_framebuffer(frame_index), sc->get_extent());
		cmd_buffer[frame_index]->bind_pipeline(p);
		cmd_buffer[frame_index]->set_viewport(sc->get_viewport(), sc->get_scissor());
		cmd_buffer[frame_index]->bind_descriptor_set(p->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
		m->draw(cmd_buffer[frame_index]);
		cmd_buffer[frame_index]->end_render_pass();
		cmd_buffer[frame_index]->end_recording();

		camera->freemove(window, delta * 2);
		camera->update_rot(window, 1, window_focused);
		
		if(!sc->image_render(queues[QUEUE_GRAPHICS], cmd_buffer[frame_index])) return 1;
		sc->image_present(queues[QUEUE_PRESENT]);

		frames++;
	}
	
	vkDeviceWaitIdle(get_device());
	
	for(size_t i = 0; i < num_images; i++)
	{
		delete cmd_buffer[i];
	}

	vkDestroyImageView(get_device(), depth_buffer_view, nullptr);
	alloc::free(depth_buffer);
	
	delete camera;
	delete[] cmd_buffer;
	delete m;
	delete p;
	delete tex;
	delete[] img_data;
	delete desc;
	unload_command_pools(command_pools);
	delete rp;
	delete sc;
	alloc::deinit();
	deinit_vulkan_application();
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}