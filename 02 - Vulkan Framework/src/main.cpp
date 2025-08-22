#include <iostream>

#include "renderer/cmdbuffer.h"
#include "renderer/descriptor.h"
#include "renderer/pipeline.h"
#include "renderer/renderpass.h"
#include "renderer/swapchain.h"
#include "renderer/texture.h"
#include "renderer/vksetup.h"

#include "utils/alloc.h"
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

static float vertices[] = 
{
	-0.5, -0.5,   1, 1, 1,   0, 0,
	 0.5, -0.5,   0, 1, 0,   1, 0,
	 0.5,  0.5,   0, 0, 1,   1, 1,
	-0.5,  0.5,   0, 1, 1,   0, 1
};

static uint16_t indices[] =
{
	0, 1, 2,
	0, 2, 3
};

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

void update_uniforms(double time, descriptor* desc)
{
	math::mat rotation = math::rotation(math::vec3(0, 0, time));
	math::mat transform = math::transform(math::vec3(std::sin(time / 4) / 2, 0, 0), rotation, math::vec3(1, 1, 1));
	
	float transform_data[3][16];
	
	for(uint8_t i = 0; i < 3; i++)
	{
		transform.get_data(transform_data[i]);
		desc->place_data(i, 0, transform_data[i]);
	}
}

void record_command_buffer(command_buffer* buffer, pipeline* p, render_pass* rp, swapchain* sc, VkFramebuffer framebuffer, VkBuffer vertex_buffer, VkBuffer index_buffer, descriptor* desc)
{
	buffer->begin_recording();
	buffer->begin_render_pass(rp, framebuffer, sc->get_extent());
	buffer->bind_pipeline(p);
	buffer->bind_vertex_buffer(vertex_buffer, 0);
	buffer->bind_index_buffer(index_buffer, 0);
	buffer->set_viewport(sc->get_viewport(), sc->get_scissor());
	buffer->bind_descriptor_set(p->get_layout(), desc->get_descriptor_set(sc->get_current_image_index()));
	buffer->draw_indexed(6, 1);
	buffer->end_render_pass();
	buffer->end_recording();
}

int main()
{
	glfwInit();
	
	//GLFW assumes you're using an OpenGL context. This hint disables said assumption.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(GLFW_DEFAULT_WIDTH, GLFW_DEFAULT_HEIGHT, "GLFW Window", NULL, NULL);
	
	if(!init_vulkan_application(window)) return 1;
	
	swapchain* sc = new swapchain(window);
	render_pass* rp = new render_pass(sc->get_format());

	uint32_t frame_count = sc->get_image_count();
	
	if(!sc->create_framebuffers(rp)) return 1;
	
	std::vector<VkShaderModule> shader_modules;
	if(!load_shaders(&shader_modules)) return 1;
	
	std::vector<VkCommandPool> command_pools;
	if(!load_command_pools(&command_pools)) return 1;
	
	std::vector<VkQueue> queues;
	get_vulkan_queues(&queues);
	
	alloc::init(queues[QUEUE_GRAPHICS], command_pools[0]);
	
	pipeline_vertex_input pvi;
	pvi.vertex_binding = create_vertex_input_binding(0, 7 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	pvi.vertex_attribs.resize(3);
	pvi.vertex_attribs[0] = create_vertex_input_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
	pvi.vertex_attribs[1] = create_vertex_input_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 2 * sizeof(float));
	pvi.vertex_attribs[2] = create_vertex_input_attribute(0, 2, VK_FORMAT_R32G32_SFLOAT, 5 * sizeof(float));

	uint32_t img_width, img_height, img_channels;
	char* img_data;
	if(!utils::load_bmp_texture("../res/textures/smile.bmp", &img_width, &img_height, &img_channels, &img_data)) return 1;

	texture* tex = new texture(img_data, img_width, img_height, VK_FORMAT_B8G8R8A8_SRGB);

	descriptor* desc = new descriptor(frame_count);

	desc->add_descriptor_binding_buffer(16 * sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);
	desc->add_descriptor_binding_sampler(tex, VK_SHADER_STAGE_FRAGMENT_BIT);

	desc->build();
	
	pipeline* p = new pipeline();
	p->add_shader_module(shader_modules[0], VK_SHADER_STAGE_VERTEX_BIT, "main");
	p->add_shader_module(shader_modules[1], VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	
	p->add_viewport(sc->get_default_viewport(), sc->get_full_scissor());
	p->add_color_blend_state(get_color_blend_attachment_none());
	p->set_pipeline_vertex_input_state(pvi);
	p->add_descriptor_set_layout(desc->get_descriptor_set_layout());
		
	if(!p->build(rp)) return 1;
	
	uint32_t num_images = sc->get_image_count();
	
	command_buffer** cmd_buffer = new command_buffer*[num_images];
	for(size_t i = 0; i < num_images; i++)
		cmd_buffer[i] = new command_buffer(command_pools[0]);
	
	alloc::buffer vertex_buffer, index_buffer;
	
	alloc::new_buffer(&vertex_buffer, vertices, 28 * sizeof(float),   ALLOC_USAGE_STAGED_VERTEX_BUFFER);
	alloc::new_buffer(&index_buffer,  indices,  6 * sizeof(uint16_t), ALLOC_USAGE_STAGED_INDEX_BUFFER);
	
	double timer = glfwGetTime();
	uint32_t frames = 0;

	double delta_timer = glfwGetTime();
	double update_time = 0;

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

		update_uniforms(update_time, desc);
		
		uint32_t frame_index;

		bool should_retry = true;
		while(should_retry) sc->retrieve_next_image(&frame_index, &should_retry);

		cmd_buffer[frame_index]->reset();
		record_command_buffer(cmd_buffer[frame_index], p, rp, sc, sc->get_framebuffer(frame_index), vertex_buffer.vk_buffer, index_buffer.vk_buffer, desc);
		
		sc->image_render(queues[QUEUE_GRAPHICS], cmd_buffer[frame_index]);
		sc->image_present(queues[QUEUE_PRESENT]);

		frames++;
	}
	
	vkDeviceWaitIdle(get_device());
	
	alloc::free(index_buffer);
	alloc::free(vertex_buffer);
	
	for(size_t i = 0; i < num_images; i++)
	{
		delete cmd_buffer[i];
	}
	
	delete[] cmd_buffer;
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