#include "../include/vulkan/vulkan.h"
#include "../include/GLFW/glfw3.h"

#include <iostream>
#include <vector>

#include "rendering/vksetup.h"
#include "rendering/swapchain.h"
#include "rendering/renderpass.h"
#include "rendering/pipeline.h"
#include "rendering/vkcommands.h"

#define MAX_FRAMES_TRANSIT 2
static uint32_t current_frame;

static GLFWwindow* window;
static VkDevice device;

static Swapchain* swapchain;
static RenderPass* rp_color;
static Pipeline* pipeline;

static CommandPool* command_pool;
static VkCommandBuffer command_buffer[MAX_FRAMES_TRANSIT];

static VkSemaphore sp_image_retrieved[MAX_FRAMES_TRANSIT], sp_render_finished[MAX_FRAMES_TRANSIT];
static VkFence fence_frame_finished[MAX_FRAMES_TRANSIT];

static VkQueue queue_graphics, queue_present;

static bool fb_resized;
static int fb_width, fb_height;

//Callback for the window size.
void callback_framebuffer_size(GLFWwindow* window, int width, int height)
{
    fb_resized = true;

    fb_width = width;
    fb_height = height;
}

//Recording pre-defined graphics/drawing commands into each command buffer.
bool record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo info_begin{};

    info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result = vkBeginCommandBuffer(command_buffer, &info_begin);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to begin recording the command buffer." << std::endl;
        return false;
    }

    VkRenderPassBeginInfo info_begin_render_pass = rp_color->create_default_render_pass_begin_info(image_index);

    VkClearValue clear_color = {{{0, 0, 0, 1}}};
    info_begin_render_pass.clearValueCount = 1;
    info_begin_render_pass.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &info_begin_render_pass, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

    VkViewport viewport;
    VkRect2D scissor;

    swapchain->get_swapchain_viewport_and_scissor(&viewport, &scissor);

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to finish recording the command buffer." << std::endl;
        return false;
    }

    return true;
}

//Sets up the Vulkan graphics pipeline.
bool create_graphics_pipeline()
{
    VkShaderModule vs_module, fs_module;

    if(!vksetup::load_shader_module(&device, &vs_module, "src/shaders/vertex.spv")) return false;
    if(!vksetup::load_shader_module(&device, &fs_module, "src/shaders/fragment.spv")) return false;

    VkPipelineShaderStageCreateInfo info_shader[2];

    info_shader[0] = {};
    info_shader[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info_shader[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    info_shader[0].module = vs_module;
    info_shader[0].pName = "main";

    info_shader[1] = {};
    info_shader[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info_shader[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    info_shader[1].module = fs_module;
    info_shader[1].pName = "main";

    pipeline = new DefaultColorPipeline(&device, rp_color, swapchain);
    pipeline->add_shader_module(info_shader[0]);
    pipeline->add_shader_module(info_shader[1]);
    pipeline->compile_pipeline();

    vkDestroyShaderModule(device, vs_module, nullptr);
    vkDestroyShaderModule(device, fs_module, nullptr);

    return true;
}

//Creates synchronization objects used for the draw loop.
bool create_draw_sync_objects()
{
    for(uint32_t i = 0; i < MAX_FRAMES_TRANSIT; i++)
    {
        if(!vksetup::create_semaphore(&device, &sp_image_retrieved[i])) return false;
        if(!vksetup::create_semaphore(&device, &sp_render_finished[i])) return false;

        if(!vksetup::create_fence(&device, &fence_frame_finished[i], true)) return false;
    }

    return true;
}

bool draw()
{
    vkWaitForFences(device, 1, &fence_frame_finished[current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    int acq_code = swapchain->acquire_next_image(&sp_image_retrieved[current_frame], &image_index);
    if(acq_code == 0)
    {
        //Recreate the semaphore, to reset it to an unsignaled state.
        vkDestroySemaphore(device, sp_image_retrieved[current_frame], nullptr);
        if(!vksetup::create_semaphore(&device, &sp_image_retrieved[current_frame])) return false;
        
        return true;
    }
    else if(acq_code == -1) return false;

    //Fence should only be reset if work is being submitted.
    vkResetFences(device, 1, &fence_frame_finished[current_frame]);

    vkResetCommandBuffer(command_buffer[current_frame], 0);
    record_command_buffer(command_buffer[current_frame], image_index);

    VkSubmitInfo info_submit{};

    info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    info_submit.waitSemaphoreCount = 1;
    info_submit.pWaitSemaphores = &sp_image_retrieved[current_frame];
    info_submit.pWaitDstStageMask = wait_stages;

    info_submit.commandBufferCount = 1;
    info_submit.pCommandBuffers = &command_buffer[current_frame];

    info_submit.signalSemaphoreCount = 1;
    info_submit.pSignalSemaphores = &sp_render_finished[current_frame];

    if(!vksetup::submit_to_queue(&queue_graphics, &info_submit, &fence_frame_finished[current_frame])) return false;

    VkPresentInfoKHR info_present{};
    info_present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    info_present.waitSemaphoreCount = 1;
    info_present.pWaitSemaphores = &sp_render_finished[current_frame];

    VkSwapchainKHR swapchains[] = {swapchain->get_swapchain()};
    info_present.swapchainCount = 1;
    info_present.pSwapchains = swapchains;
    info_present.pImageIndices = &image_index;

    vkQueuePresentKHR(queue_present, &info_present);

    current_frame++;
    current_frame %= MAX_FRAMES_TRANSIT;

    return true;
}

//Deallocates and destroys all Vulkan resources before the program terminates.
void clean()
{
    for(uint32_t i = 0; i < MAX_FRAMES_TRANSIT; i++)
    {
        vkDestroySemaphore(device, sp_image_retrieved[i], nullptr);
        vkDestroySemaphore(device, sp_render_finished[i], nullptr);
        vkDestroyFence(device, fence_frame_finished[i], nullptr);
    }
    
    delete command_pool;
    delete pipeline;
    delete rp_color;
    delete swapchain;
    
    vkDestroyDevice(device, nullptr);
    vksetup::destroy_vulkan();

    glfwDestroyWindow(window);
    glfwTerminate();
}

//Creating all necessary Vulkan resources, the swapchain, the render pass and command buffers.
//Also initializing the GLFW window.
int init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    fb_resized = false;
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    current_frame = 0;

    window = glfwCreateWindow(800, 600, "Test window", NULL, NULL);
    
    if(!vksetup::setup_vulkan(&device, window));
    
    swapchain = new Swapchain(&device, window);
    if(!swapchain->is_usable()) return 1;

    rp_color = new DefaultColorRenderPass(&device, swapchain);
    if(!rp_color->is_usable()) return 1;

    if(!create_graphics_pipeline()) return 1;

    command_pool = new CommandPool(&device);
    if(!command_pool->is_usable()) return 1;

    for(uint32_t i = 0; i < MAX_FRAMES_TRANSIT; i++)
    {
        if(!command_pool->allocate_command_buffer(&command_buffer[i], true)) return 1;
    }

    if(!create_draw_sync_objects()) return 1;

    return 0;
}

int main()
{
    if(init()) return 1;

    vksetup::queue_family_indices qf_indices = vksetup::find_physical_device_queue_families();

    vkGetDeviceQueue(device, qf_indices.graphics_index.value(), 0, &queue_graphics);
    vkGetDeviceQueue(device, qf_indices.presentation_index.value(), 0, &queue_present);

    int frames = 0;
    double time = glfwGetTime();

    while(!glfwWindowShouldClose(window))
    {
        if(glfwGetTime() - time >= 1.0)
        {
            std::cout << "FPS: " << frames << std::endl;
            frames = 0;
            time += 1.0;
        }

        glfwPollEvents();
        if(!draw()) return 1;
        
        frames++;
    }
    
    vkDeviceWaitIdle(device);

    clean();
    
    return 0;
}