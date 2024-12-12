#include "../include/vulkan/vulkan.h"
#include "../include/GLFW/glfw3.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <vector>

#define VALIDATION_LAYER_COUNT 1
#define DEVICE_EXTENSION_COUNT 1

const char* validation_layers[VALIDATION_LAYER_COUNT] =
{
    "VK_LAYER_KHRONOS_validation"
};

const char* device_extensions[DEVICE_EXTENSION_COUNT] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef BUILD_DEBUG
    const bool validate = true;
#else
    const bool validate = false;
#endif

static VkDebugUtilsMessengerEXT vl_debug_messenger;

static GLFWwindow* window;

static VkInstance instance;
static VkPhysicalDevice physical_device;
static VkDevice device;

static VkSurfaceKHR surface;
static VkSwapchainKHR swapchain;
static VkSurfaceFormatKHR swapchain_format;
static VkExtent2D swapchain_extent;

static std::vector<VkImage> swapchain_images;
static std::vector<VkImageView> swapchain_image_views;
static std::vector<VkFramebuffer> framebuffers;

static VkRenderPass render_pass;
static VkPipeline pipeline;
static VkPipelineLayout pipeline_layout;

static VkCommandPool command_pool;
static VkCommandBuffer command_buffer;

static VkSemaphore sp_image_retrieved, sp_render_finished;
static VkFence fence_frame_finished;

static VkQueue queue_graphics, queue_present;

//Callback function for when the validation layer is triggered. Prints out the layer's message or response.
static VKAPI_ATTR VkBool32 VKAPI_CALL vl_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    if(message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT && message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return VK_FALSE;
    
    std::cerr << "[VULKAN VL] " << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

//Returns a VkDebugUtilsMessengerCreateInfoEXT struct populated with the desired parameters set by me.
//Made this into its own function since we technically need two messengers - one for the general validation
//layer handle, and one specifically to handle instance creation and deletion, which occur outside the lifetime
//of the validation layer handle.
VkDebugUtilsMessengerCreateInfoEXT get_global_vl_messenger_info()
{
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    info.pfnUserCallback = vl_debug_callback;

    return info;
}

//Checking if the current device supports the required validation layers. Is only called if
//BUILD_DEBUG is defined.
bool check_validation_layer_support()
{
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    VkLayerProperties* layers = new VkLayerProperties[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    for(uint32_t i = 0; i < VALIDATION_LAYER_COUNT; i++)
    {
        bool found_layer = false;
        for(uint32_t j = 0; j < layer_count; j++)
        {
            if(strcmp(layers[j].layerName, validation_layers[i]) == 0)
            {
                found_layer = true;
                break;
            }
        }

        if(!found_layer)
        {
            std::cerr << "[ERR] Could not locate validation layer: " << validation_layers[i] << std::endl;

            delete[] layers;
            return false;
        }
    }

    delete[] layers;
    return true;
}

//Verifying the availability of and getting the names for every required Vulkan extension for this program.
//If BUILD_DEBUG is defined, then the VK_EXT_debug_utils extension is included to print validation info.
bool get_required_extensions(const char*** ext, uint32_t* ext_count)
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    VkExtensionProperties* extensions = new VkExtensionProperties[extension_count];
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions);

    uint32_t required_extension_count = 0;
    uint32_t glfw_extension_count = 0;
    const char** glfw_req_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    required_extension_count = glfw_extension_count;
    
    if(validate) required_extension_count++;
    
    const char** required_extensions = new const char*[required_extension_count];
    for(uint32_t i = 0; i < glfw_extension_count; i++)
    {
        required_extensions[i] = glfw_req_extensions[i];
    }

    if(validate)
    {
        required_extensions[glfw_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    for(uint32_t i = 0; i < required_extension_count; i++)
    {
        bool found_extension = false;
        for(uint32_t j = 0; j < extension_count; j++)
        {
            if(strcmp(extensions[j].extensionName, required_extensions[i]) == 0)
            {
                found_extension = true;
                break;
            }
        }

        if(!found_extension)
        {
            std::cerr << "[ERR] Could not locate extension: " << required_extensions[i] << std::endl;

            delete[] extensions;
            delete[] required_extensions;

            *ext_count = 0;
            *ext = nullptr;

            return false;
        }
    }

    delete[] extensions;

    *ext = required_extensions;
    *ext_count = required_extension_count;

    return true;
}

typedef struct
{
    //These values are optionals to indicate if the GPU even has these queue families.
    std::optional<uint32_t> graphics_index;
    std::optional<uint32_t> presentation_index;
} queue_family_indices;

//Locates the queue families on a GPU.
queue_family_indices find_physical_device_queue_families(VkPhysicalDevice device)
{
    queue_family_indices indices;
    
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    for(uint32_t i = 0; i < queue_family_count; i++)
    {
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_index = i;
        }
        
        VkBool32 supports_presentation;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_presentation);

        if(supports_presentation)
        {
            indices.presentation_index = i;
        }
    }

    delete[] queue_families;
    return indices;
}

//Checking if a VkPhysicalDevice supports the required extensions for the program.
bool check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    if(extension_count == 0) return false;

    VkExtensionProperties* extensions = new VkExtensionProperties[extension_count];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions);

    std::set<std::string> req_extensions;
    for(uint32_t i = 0; i < DEVICE_EXTENSION_COUNT; i++)
    {
        req_extensions.insert(device_extensions[i]);
    }

    for(uint32_t i = 0; i < extension_count; i++)
    {
        req_extensions.erase(extensions[i].extensionName);
    }

    delete[] extensions;
    return req_extensions.empty();
}

typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentation_modes;
} swap_chain_support;

//Getting the supported capabilities, formats, and presentation modes for the swap chain on a physical device.
swap_chain_support query_swap_chain_support(VkPhysicalDevice device)
{
    swap_chain_support support;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.capabilities);

    uint32_t format_count, present_mode_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if(format_count != 0)
    {
        support.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, support.formats.data());
    }

    if(present_mode_count != 0)
    {
        support.presentation_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, support.presentation_modes.data());
    }

    return support;
}

//Chooses a supported format for the swap chain to use.
VkSurfaceFormatKHR choose_swap_chain_format(swap_chain_support supported)
{
    for(auto format : supported.formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return supported.formats[0];
}

//Chooses a supported presentation mode for the swap chain to use.
VkPresentModeKHR choose_swap_chain_present_mode(swap_chain_support supported)
{
    for(auto present_mode : supported.presentation_modes)
    {
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//Chooses a supported and/or desired resolution (extent) for the swap chain images.
VkExtent2D choose_swap_chain_extent(swap_chain_support supported)
{
    auto& cap = supported.capabilities;
    if(cap.currentExtent.width != std::numeric_limits<uint32_t>::max())
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

//Loads a compiled shader module, specified by filepath.
bool load_shader_module(VkShaderModule* module, const char* filepath)
{
    std::ifstream reader(filepath, std::ios::binary);
    if(!reader.good())
    {
        std::cerr << "[ERR] Cannot read shader module: " << filepath << std::endl;
        return false;
    }

    uint32_t length;
    reader.seekg(0, reader.end);
    length = reader.tellg();
    reader.seekg(0, reader.beg);

    char* code = new char[length];
    reader.read(code, length);

    reader.close();

    VkShaderModuleCreateInfo info_module{};
    info_module.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info_module.codeSize = length;
    info_module.pCode = reinterpret_cast<const uint32_t*>(code);

    VkResult result = vkCreateShaderModule(device, &info_module, nullptr, module);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create shader module. (" << filepath << ")." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        delete[] code;
        return false;
    }

    delete[] code;
    return true;
}

//Creates a render pass, which is used for each framebuffer, and so also creates all of the framebuffer handles.
bool create_render_pass()
{
    VkAttachmentDescription attachment_color{};
    attachment_color.format = swapchain_format.format;
    attachment_color.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachment_color_reference{};
    attachment_color_reference.attachment = 0;
    attachment_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_color{};
    subpass_color.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_color.colorAttachmentCount = 1;
    subpass_color.pColorAttachments = &attachment_color_reference;

    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info_render_pass{};
    info_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info_render_pass.attachmentCount = 1;
    info_render_pass.pAttachments = &attachment_color;
    info_render_pass.subpassCount = 1;
    info_render_pass.pSubpasses = &subpass_color;
    info_render_pass.dependencyCount = 1;
    info_render_pass.pDependencies = &subpass_dependency;

    VkResult result = vkCreateRenderPass(device, &info_render_pass, nullptr, &render_pass);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan render pass." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    framebuffers.resize(swapchain_image_views.size());

    for(uint32_t i = 0; i < framebuffers.size(); i++)
    {
        VkImageView attachments[] = {swapchain_image_views[i]};

        VkFramebufferCreateInfo info_framebuffer{};
        info_framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info_framebuffer.renderPass = render_pass;
        info_framebuffer.attachmentCount = 1;
        info_framebuffer.pAttachments = attachments;
        info_framebuffer.width = swapchain_extent.width;
        info_framebuffer.height = swapchain_extent.height;
        info_framebuffer.layers = 1;

        VkResult result = vkCreateFramebuffer(device, &info_framebuffer, nullptr, &framebuffers[i]);
        if(result != VK_SUCCESS)
        {
            std::cerr << "[ERR] Failed to create framebuffer." << std::endl;
            std::cerr << "[ERR] Error code: " << result << std::endl;
            return false;
        }
    }

    return true;
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

    VkRenderPassBeginInfo info_begin_render_pass{};
    info_begin_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info_begin_render_pass.renderPass = render_pass;
    info_begin_render_pass.framebuffer = framebuffers[image_index];
    info_begin_render_pass.renderArea.offset = {0, 0};
    info_begin_render_pass.renderArea.extent = swapchain_extent;

    VkClearValue clear_color = {{{0, 0, 0, 1}}};
    info_begin_render_pass.clearValueCount = 1;
    info_begin_render_pass.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &info_begin_render_pass, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

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

//Initializes Vulkan and creates a Vulkan instance with the required extensions to use. If BUILD_DEBUG
//is defined, also adds validation layers for verbose error checking.
bool init_vulkan_instance()
{
    if(validate && !check_validation_layer_support())
    {
        std::cerr << "[ERR] Validation layers requested, but not available." << std::endl;
        return false;
    }

    const char** extensions;
    uint32_t extension_count;
    if(!get_required_extensions(&extensions, &extension_count))
    {
        std::cerr << "[ERR] Requested extensions unavailable." << std::endl;
        return false;
    }

    VkApplicationInfo info_app{};
    info_app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    info_app.pApplicationName = "Hello Triangle";
    info_app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    info_app.pEngineName = "No Engine";
    info_app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    info_app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo info_inst_create{};
    info_inst_create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info_inst_create.pApplicationInfo = &info_app;
    info_inst_create.enabledExtensionCount = extension_count;
    info_inst_create.ppEnabledExtensionNames = extensions;

    //Placed outside the if statement to ensure it doesn't go out-of-scope before the vkCreateInstance call.
    auto messenger_info = get_global_vl_messenger_info();

    if(validate)
    {
        info_inst_create.enabledLayerCount = 1;
        info_inst_create.ppEnabledLayerNames = validation_layers;

        info_inst_create.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &messenger_info;
    }
    else
    {
        info_inst_create.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&info_inst_create, nullptr, &instance);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan instance." << std::endl;
        std::cerr << "Error code: " << result << std::endl;
        return false;
    }

    delete[] extensions;

    return true;
}

//If BUILD_DEBUG is defined, applies the vl_debug_callback function as the callback function for
//when the validation layer has output.
bool setup_debug_messenger()
{
    if(!validate) return true;

    auto info = get_global_vl_messenger_info();

    //Extension functions are not automatically loaded - we have to get their address manually to call them.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func == nullptr)
    {
        std::cerr << "[ERR] Extension function \'vkCreateDebugUtilsMessengerEXT\' is inaccessable." << std::endl;
        return false;
    }

    VkResult result = func(instance, &info, nullptr, &vl_debug_messenger);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan debug messenger." << std::endl;
        std::cerr << "Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Creating a window surface for the GLFW context. The window surface is a handle which will allow Vulkan
//to display images from a swap chain to the window surface.
bool create_window_surface()
{
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan window surface for GLFW." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Selects a graphics card or other GPU to use for Vulkan operations, and loads a logical device (VkDevice)
//handle to tell Vulkan which device features the program is planning to use.
bool select_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, 0);

    if(device_count == 0)
    {
        std::cerr << "[ERR] Unable to locate any GPUs that support Vulkan." << std::endl;
        return false;
    }

    VkPhysicalDevice* physical_devices = new VkPhysicalDevice[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    uint32_t max_score = 0;
    uint32_t current_selection = 0;

    //Scoring each device to determine which is the most suitable.
    for(uint32_t i = 0; i < device_count; i++)
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;

        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
        
        uint32_t score = 0;

        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 10;
        if(features.geometryShader) score += 3; //Unnessecary but I'm doing it for demonstration purposes.

        queue_family_indices indices = find_physical_device_queue_families(physical_devices[i]);
        if(!indices.graphics_index.has_value() || !check_device_extension_support(physical_devices[i])) score = 0; //Unusable.

        swap_chain_support scs = query_swap_chain_support(physical_devices[i]);
        if(scs.formats.size() == 0 || scs.presentation_modes.size() == 0) score = 0; //No supported formats or presentation modes for the swap chain.

        if(score > max_score)
        {
            current_selection = i;
            max_score = score;
        }
    }

    if(max_score == 0 && current_selection == 0)
    {
        std::cerr << "[ERR] Couldn't find a suitable graphics device." << std::endl;
        delete[] physical_devices;
        return false;
    }

    physical_device = physical_devices[current_selection];
    delete[] physical_devices;

    queue_family_indices selected_indices = find_physical_device_queue_families(physical_device);
    std::set<uint32_t> queue_indices = {selected_indices.graphics_index.value(), selected_indices.presentation_index.value()};
    VkDeviceQueueCreateInfo* info_queue = new VkDeviceQueueCreateInfo[queue_indices.size()];

    float queue_priority = 1.f;

    uint32_t i = 0;
    for(uint32_t index : queue_indices)
    {
        info_queue[i] = {};
        info_queue[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info_queue[i].queueFamilyIndex = index;
        info_queue[i].queueCount = 1;
        info_queue[i].pQueuePriorities = &queue_priority;
        i++;
    }

    VkPhysicalDeviceFeatures info_features{};

    VkDeviceCreateInfo info_device{};
    info_device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info_device.queueCreateInfoCount = queue_indices.size();
    info_device.pQueueCreateInfos = info_queue;
    info_device.pEnabledFeatures = &info_features;
    info_device.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
    info_device.ppEnabledExtensionNames = device_extensions;

    //Technically optional - the modern specification no longer uses device-specific validation layers and will ignore these values.
    //Still worth implementing for older drivers/hardware.
    if(validate)
    {
        info_device.enabledLayerCount = 1;
        info_device.ppEnabledLayerNames = validation_layers;
    }
    else
    {
        info_device.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physical_device, &info_device, nullptr, &device);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create logical Vulkan device." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        delete[] info_queue;
        return false;
    }

    delete[] info_queue;
    return true;
}

//Creating the swap chain for Vulkan to use to present images to the window.
bool create_swap_chain()
{
    swap_chain_support supported_features = query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR sc_format = choose_swap_chain_format(supported_features);
    VkPresentModeKHR sc_present_mode = choose_swap_chain_present_mode(supported_features);
    VkExtent2D sc_extent = choose_swap_chain_extent(supported_features);

    uint32_t sc_max_image_count = supported_features.capabilities.maxImageCount;

    uint32_t sc_image_count = supported_features.capabilities.minImageCount + 1;
    if(sc_image_count > sc_max_image_count && sc_max_image_count != 0)
        sc_image_count = sc_max_image_count;
    
    VkSwapchainCreateInfoKHR info_swapchain{};
    info_swapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info_swapchain.surface = surface;
    info_swapchain.presentMode = sc_present_mode;
    info_swapchain.minImageCount = sc_image_count;
    info_swapchain.imageFormat = sc_format.format;
    info_swapchain.imageColorSpace = sc_format.colorSpace;
    info_swapchain.imageExtent = sc_extent;
    info_swapchain.imageArrayLayers = 1;
    info_swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queue_family_indices indices = find_physical_device_queue_families(physical_device);
    uint32_t qfi[] = {indices.graphics_index.value(), indices.presentation_index.value()};

    if(indices.graphics_index == indices.presentation_index)
        info_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    else
    {
        info_swapchain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info_swapchain.queueFamilyIndexCount = 2;
        info_swapchain.pQueueFamilyIndices = qfi;
    }

    info_swapchain.preTransform = supported_features.capabilities.currentTransform;
    info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info_swapchain.clipped = VK_TRUE;

    info_swapchain.oldSwapchain = VK_NULL_HANDLE; //We'll come back to this.

    VkResult result = vkCreateSwapchainKHR(device, &info_swapchain, nullptr, &swapchain);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create Vulkan swap chain." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    swapchain_format = sc_format;
    swapchain_extent = sc_extent;

    vkGetSwapchainImagesKHR(device, swapchain, &sc_image_count, nullptr);
    swapchain_images.resize(sc_image_count);
    swapchain_image_views.resize(sc_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &sc_image_count, swapchain_images.data());

    for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
    {
        VkImageViewCreateInfo info_image_view{};
        info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info_image_view.image = swapchain_images[i];
        info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info_image_view.format = swapchain_format.format;
        info_image_view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info_image_view.subresourceRange.baseMipLevel = 0;
        info_image_view.subresourceRange.levelCount = 1;
        info_image_view.subresourceRange.baseArrayLayer = 0;
        info_image_view.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &info_image_view, nullptr, &swapchain_image_views[i]);
        if(result != VK_SUCCESS)
        {
            std::cerr << "[ERR] Failed to create Vulkan image view." << std::endl;
            std::cerr << "[ERR] Error code: " << result << std::endl;
            return false;
        }
    }

    return true;
}

//Sets up the Vulkan graphics pipeline.
bool create_graphics_pipeline()
{
    VkShaderModule vs_module, fs_module;

    if(!load_shader_module(&vs_module, "src/shaders/vertex.spv")) return false;
    if(!load_shader_module(&fs_module, "src/shaders/fragment.spv")) return false;

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

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo info_dynamic_state{};
    info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info_dynamic_state.dynamicStateCount = 2;
    info_dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineVertexInputStateCreateInfo info_vertex_input{};
    info_vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info_vertex_input.vertexBindingDescriptionCount = 0;
    info_vertex_input.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo info_input_assembly{};
    info_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info_input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.height;
    viewport.minDepth = -1;
    viewport.maxDepth = 1;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

    VkPipelineViewportStateCreateInfo info_viewport{};
    info_viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info_viewport.viewportCount = 1;
    info_viewport.pViewports = &viewport;
    info_viewport.scissorCount = 1;
    info_viewport.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo info_rasterizer{};
    info_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    info_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    info_rasterizer.lineWidth = 1;
    info_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    info_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info_rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo info_multisample{};
    info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info_multisample.sampleShadingEnable = VK_FALSE;
    info_multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState attachment_color_blend{};
    attachment_color_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment_color_blend.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo info_color_blending{};
    info_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info_color_blending.logicOpEnable = VK_FALSE;
    info_color_blending.attachmentCount = 1;
    info_color_blending.pAttachments = &attachment_color_blend;

    VkPipelineLayoutCreateInfo info_pipeline_layout{};
    info_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info_pipeline_layout.setLayoutCount = 0; //Default value - optional.

    VkResult result = vkCreatePipelineLayout(device, &info_pipeline_layout, nullptr, &pipeline_layout);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create graphics pipeline layout." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;

        vkDestroyShaderModule(device, vs_module, nullptr);
        vkDestroyShaderModule(device, fs_module, nullptr);

        return false;
    }

    VkGraphicsPipelineCreateInfo info_pipeline{};
    info_pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info_pipeline.stageCount = 2;
    info_pipeline.pStages = info_shader;
    info_pipeline.pVertexInputState = &info_vertex_input;
    info_pipeline.pInputAssemblyState = &info_input_assembly;
    info_pipeline.pViewportState = &info_viewport;
    info_pipeline.pRasterizationState = &info_rasterizer;
    info_pipeline.pMultisampleState = &info_multisample;
    info_pipeline.pDepthStencilState = nullptr;
    info_pipeline.pColorBlendState = &info_color_blending;
    info_pipeline.pDynamicState = &info_dynamic_state;
    info_pipeline.layout = pipeline_layout;
    info_pipeline.renderPass = render_pass;
    info_pipeline.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info_pipeline, nullptr, &pipeline);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create graphics pipeline." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;

        vkDestroyShaderModule(device, vs_module, nullptr);
        vkDestroyShaderModule(device, fs_module, nullptr);

        return false;
    }

    vkDestroyShaderModule(device, vs_module, nullptr);
    vkDestroyShaderModule(device, fs_module, nullptr);

    return true;
}

//Creates a command pool using the graphics queue family.
bool create_command_pool()
{
    queue_family_indices qf_indices = find_physical_device_queue_families(physical_device);

    VkCommandPoolCreateInfo info_create_pool{};

    info_create_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info_create_pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info_create_pool.queueFamilyIndex = qf_indices.graphics_index.value();

    VkResult result = vkCreateCommandPool(device, &info_create_pool, nullptr, &command_pool);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create command pool handle." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Allocates a command buffer from a command pool.
bool allocate_command_buffer()
{
    VkCommandBufferAllocateInfo info_alloc_buffer{};

    info_alloc_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info_alloc_buffer.commandPool = command_pool;
    info_alloc_buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info_alloc_buffer.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(device, &info_alloc_buffer, &command_buffer);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to allocate command buffer." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Creates synchronization objects used for the draw loop.
bool create_draw_sync_objects()
{
    VkSemaphoreCreateInfo info_create_semaphore{};
    info_create_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo info_create_fence{};
    info_create_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info_create_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateSemaphore(device, &info_create_semaphore, nullptr, &sp_image_retrieved);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create semaphore." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    result = vkCreateSemaphore(device, &info_create_semaphore, nullptr, &sp_render_finished);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create semaphore." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    result = vkCreateFence(device, &info_create_fence, nullptr, &fence_frame_finished);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create fence." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

bool draw()
{
    vkWaitForFences(device, 1, &fence_frame_finished, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence_frame_finished);

    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, sp_image_retrieved, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(command_buffer, image_index);

    VkSubmitInfo info_submit{};

    info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore wait_semaphores[] = {sp_image_retrieved};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    info_submit.waitSemaphoreCount = 1;
    info_submit.pWaitSemaphores = wait_semaphores;
    info_submit.pWaitDstStageMask = wait_stages;

    info_submit.commandBufferCount = 1;
    info_submit.pCommandBuffers = &command_buffer;

    VkSemaphore signal_semaphores[] = {sp_render_finished};

    info_submit.signalSemaphoreCount = 1;
    info_submit.pSignalSemaphores = signal_semaphores;

    VkResult result = vkQueueSubmit(queue_graphics, 1, &info_submit, fence_frame_finished);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to submit graphics commands." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    VkPresentInfoKHR info_present{};
    info_present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    info_present.waitSemaphoreCount = 1;
    info_present.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    info_present.swapchainCount = 1;
    info_present.pSwapchains = swapchains;
    info_present.pImageIndices = &image_index;
    
    vkQueuePresentKHR(queue_present, &info_present);

    return true;
}

//Deallocates and destroys all Vulkan resources before the program terminates.
void clean()
{
    vkDestroySemaphore(device, sp_image_retrieved, nullptr);
    vkDestroySemaphore(device, sp_render_finished, nullptr);
    vkDestroyFence(device, fence_frame_finished, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for(uint32_t i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }

    for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if(validate)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr) func(instance, vl_debug_messenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 600, "Test window", NULL, NULL);

    if(!init_vulkan_instance()) return 1;
    if(!setup_debug_messenger()) return 1;
    if(!create_window_surface()) return 1;
    if(!select_physical_device()) return 1;
    if(!create_swap_chain()) return 1;
    if(!create_render_pass()) return 1;
    if(!create_graphics_pipeline()) return 1;
    if(!create_command_pool()) return 1;
    if(!allocate_command_buffer()) return 1;
    if(!create_draw_sync_objects()) return 1;

    queue_family_indices qf_indices = find_physical_device_queue_families(physical_device);

    vkGetDeviceQueue(device, qf_indices.graphics_index.value(), 0, &queue_graphics);
    vkGetDeviceQueue(device, qf_indices.presentation_index.value(), 0, &queue_present);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if(!draw()) return 1;
    }
    
    vkDeviceWaitIdle(device);

    clean();

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}