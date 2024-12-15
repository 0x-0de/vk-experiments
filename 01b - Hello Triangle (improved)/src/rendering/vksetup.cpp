#include "vksetup.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <set>

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

static VkDebugUtilsMessengerEXT vl_debug_messenger;

static VkInstance instance;
static VkPhysicalDevice physical_device;

static VkSurfaceKHR surface;

static std::vector<VkSemaphore*> semaphores_in_use;
static std::vector<VkFence*> fences_in_use;

#ifdef BUILD_DEBUG
    const bool validate = true;
#else
    const bool validate = false;
#endif

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

vksetup::queue_family_indices vksetup::find_physical_device_queue_families()
{
    return find_physical_device_queue_families(physical_device);
}

//Locates the queue families on a GPU.
vksetup::queue_family_indices vksetup::find_physical_device_queue_families(VkPhysicalDevice device)
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

vksetup::swap_chain_support vksetup::query_swap_chain_support()
{
    return query_swap_chain_support(physical_device);
}

//Getting the supported capabilities, formats, and presentation modes for the swap chain on a physical device.
vksetup::swap_chain_support vksetup::query_swap_chain_support(VkPhysicalDevice device)
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

//Creating a window surface for the GLFW context. The window surface is a handle which will allow Vulkan
//to display images from a swap chain to the window surface.
bool create_window_surface(GLFWwindow* window)
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

//Selects a graphics card or other GPU to use for Vulkan operations, and loads a logical device (VkDevice)
//handle to tell Vulkan which device features the program is planning to use.
bool select_physical_device(VkDevice* device)
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

        vksetup::queue_family_indices indices = vksetup::find_physical_device_queue_families(physical_devices[i]);
        if(!indices.graphics_index.has_value() || !check_device_extension_support(physical_devices[i])) score = 0; //Unusable.

        vksetup::swap_chain_support scs = vksetup::query_swap_chain_support(physical_devices[i]);
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

    vksetup::queue_family_indices selected_indices = vksetup::find_physical_device_queue_families(physical_device);
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

    VkResult result = vkCreateDevice(physical_device, &info_device, nullptr, device);
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

bool vksetup::create_fence(VkDevice* device, VkFence* fence, bool start_signaled)
{
    VkFenceCreateInfo info_create_fence{};
    info_create_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if(start_signaled) info_create_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateFence(*device, &info_create_fence, nullptr, fence);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create fence." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

bool vksetup::create_semaphore(VkDevice* device, VkSemaphore* semaphore)
{
    VkSemaphoreCreateInfo info_create_semaphore{};
    info_create_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(*device, &info_create_semaphore, nullptr, semaphore);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to create semaphore." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Loads a compiled shader module, specified by filepath.
bool vksetup::load_shader_module(VkDevice* device, VkShaderModule* module, const char* filepath)
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

    VkResult result = vkCreateShaderModule(*device, &info_module, nullptr, module);
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

bool vksetup::setup_vulkan(VkDevice* device, GLFWwindow* window)
{
    if(!init_vulkan_instance()) return false;
    if(!setup_debug_messenger()) return false;
    if(!create_window_surface(window)) return false;
    if(!select_physical_device(device)) return false;

    return true;
}

bool vksetup::submit_to_queue(VkQueue* queue, VkSubmitInfo* info, VkFence* fence)
{
    VkResult result = vkQueueSubmit(*queue, 1, info, *fence);
    if(result != VK_SUCCESS)
    {
        std::cerr << "[ERR] Failed to submit commands." << std::endl;
        std::cerr << "[ERR] Error code: " << result << std::endl;
        return false;
    }

    return true;
}

void vksetup::destroy_vulkan()
{
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if(validate)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr) func(instance, vl_debug_messenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}

VkInstance vksetup::get_vulkan_instance()
{
    return instance;
}

VkPhysicalDevice vksetup::get_selected_physical_device()
{
    return physical_device;
}

VkSurfaceKHR vksetup::get_window_surface()
{
    return surface;
}