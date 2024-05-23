/*
This learning exercise was made possible through https://vulkan-tutorial.com. The majority of this code and the notes found here
can be sourced directly from that site.

Vulkan is a (relatively) new API created by Khronos Group, the creators of OpenGL, which saw it's first release in February 2016.
Compared to it's predecessor, OpenGL, Vulkan has less overhead and more consistent driver behavior across different platforms and
devices. Vulkan allows developers the freedom to more precisely control and optimize their graphics programs than OpenGL does, at
the price of being far more verbose and requiring the developer to define and configure every step of the graphical process, from
selecting which physical device to use in your machine (if you have multiple GPUs or drivers), to creating your own framebuffer
and buffer swapping behavior. Like OpenGL, Vulkan is designed to be cross-platform, able to be run on Windows, Linux, and even
Android devices.

The steps required to get a simple triangle drawing are documented thoroughly here, but for extra notes and resources, it's
highly recommended you visit https://vulkan-tutorial.com and read through the Vulkan docs on Khronos' official website.

If you want to understand what's happening here, visit the main() function and take a tour through each step of the Vulkan
initialization (and destruction) process from there.
*/

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include "../lib/vulkan/vulkan.h"
#include "../lib/GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "../lib/GLFW/glfw3native.h"

#ifdef APP_DEBUG
    static bool enable_validation_layers = true;
#else
    static bool enable_validation_layers = false;
#endif

#define APP_VALIDATION_LAYERS_SIZE 1
static const char* validation_layers[] =
{
    "VK_LAYER_KHRONOS_validation"
};

#define APP_REQUIRED_EXTENSIONS_SIZE 1
static const char* required_device_extensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#define MAX_FRAMES_IN_FLIGHT 2

struct QueueFamilyProperties
{
    uint32_t graphics_index;
    uint32_t presentation_index;

    bool has_graphics = false;
    bool has_presentation = false;

    bool is_complete()
    {
        return has_graphics && has_presentation;
    }
};

struct SwapChainSupportProperties
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

//Simple error-checking function to make sure that VkResult enums have the correct value.
bool validate_vulkan_result(VkResult result, const char* error)
{
    if(result != VK_SUCCESS)
    {
        std::cerr << error << std::endl;
        std::cerr << "Error code: " << result << std::endl;
        return false;
    }

    return true;
}

//Proxy function to create a DebugUtilsMessengerEXT object. Because it's an extension function, it's not automatically loaded.
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//Proxy function to destroy a DebugUtilsMessengerEXT object. Because it's an extension function, it's not automatically loaded.
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

//This is the debug validation layer callback method. It's optional, as validation layers will print debug messages to the standard output by
//default, but this creates an explicit callback that allows you to specify what kinds of messages you'd like to see (not all of them indicate
//fatal errors).
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT msg_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* data_callback,
                                                     void* data_user)
{
    //The first parameter indicates the severity of the message.
    /*
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message.
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message.
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Warning message (not an error, but a potential source of one).
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Error message - behavior is invalid.
    */

    //The second parameter can have the following values:
    /*
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Something occurred which is unrelated to the below options.
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something violates the specification or indicates a possible mistake.
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan.
    */

    std::cerr << "[VAL]: " << data_callback->pMessage << std::endl;
    return VK_FALSE;
}

bool setup_debug_messenger(VkInstance* instance, VkDebugUtilsMessengerEXT* messenger)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    //The messageSeverity field lets you specify the types of severities that the callback should include.
    debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    
    //Similarly, the messageType field lets you specify the types of messages the callback is notified about.
    debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    
    debug_messenger_create_info.pfnUserCallback = debug_callback; //Specifies the callback function to use.

    if(!validate_vulkan_result(CreateDebugUtilsMessengerEXT(*instance, &debug_messenger_create_info, nullptr, messenger), "Failed to set up debug messenger callback!"))
    {
        return false;
    }

    return true;
}

//Checking to see if the requested validation layers are supported on the system.
bool check_validation_layer_support()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    if(layer_count == 0) return false;

    VkLayerProperties* layers = new VkLayerProperties[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    for(uint32_t i = 0; i < APP_VALIDATION_LAYERS_SIZE; i++)
    {
        bool found = false;
        for(uint32_t j = 0; j < layer_count; j++)
        {
            if(strcmp(validation_layers[i], layers[j].layerName) == 0)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            delete[] layers;
            return false;
        }
    }

    delete[] layers;
    return true;
}

GLFWwindow* init_window()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //GLFW creates an OpenGL context by default, that is disabled with this flag.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //It may be recommended to disable window resizing to start since that needs to be handled with care under Vulkan.

    return glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
}

bool read_spv_file(char** data, size_t* data_size, const char* filepath)
{
    std::ifstream reader(filepath, std::ios::binary);
    if(!reader.good())
    {
        std::cerr << "Failed to read file: " << filepath << std::endl;
        return false;
    }
    reader.seekg(0, reader.end);
    *data_size = reader.tellg();

    *data = new char[*data_size];

    reader.seekg(0, reader.beg);
    reader.read(*data, *data_size);

    reader.close();

    return true;
}

bool create_semaphores(std::vector<VkSemaphore>* semaphores, VkDevice* device)
{
    size_t count = semaphores->size();

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(uint32_t i = 0; i < count; i++)
    {
        if(vkCreateSemaphore(*device, &semaphore_create_info, nullptr, semaphores->data() + i) != VK_SUCCESS)
        {
            std::cerr << "Failed to create semaphore." << std::endl;
            return false;
        }
    }

    return true;
}

bool create_fences(std::vector<VkFence>* fences, VkDevice* device)
{
    size_t count = fences->size();

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < count; i++)
    {
        if(vkCreateFence(*device, &fence_create_info, nullptr, fences->data() + i) != VK_SUCCESS)
        {
            std::cerr << "Failed to create fence." << std::endl;
            return false;
        }
    }

    return true;
}

bool create_vulkan_instance(VkInstance* instance)
{
    //Validation layers are used for debugging and error-checking Vulkan allocations and other API calls. This setup has them enabled with the APP_DEBUG
    //macro, and uses the implemented validation layers that are a part of the LunarG Vulkan SDK.

    if(enable_validation_layers && !check_validation_layer_support())
    {
        std::cerr << "Requested validation layers are either not supported or unavailable." << std::endl;
        return false;
    }

    //Checking if the necessary validation layers are supported...

    //The VkApplicationInfo is an optional struct that might provide some useful information to the driver in order to optimize the application.
    //IMPORTANT: Remember to initialize your structs with {}. Not doing so will likely lead to structs containing garbage data and producing errors.
    VkApplicationInfo application_info{};

    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "Vulkan Test";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); //Vulkan comes with a macro that generates an int value representing a version number.
    application_info.apiVersion = VK_API_VERSION_1_0;

    //This struct is required for creating the VkInstance object, by providing some information about the instance.
    VkInstanceCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;

    //Because Vulkan is platform-agnostic, an extension is needed to interface with the OS's window system. GLFW has a built-in function which returns the
    //extensions required to interface with the library.

    uint32_t glfw_extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extensions_count);
    if(enable_validation_layers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    if(enable_validation_layers)
    {
        create_info.enabledLayerCount = APP_VALIDATION_LAYERS_SIZE;
        create_info.ppEnabledLayerNames = validation_layers;
    }
    else create_info.enabledLayerCount = 0;

    //This function actually creates the VkInstance, returning a VkResult.
    //Most vkCreateXXX functions follow the same pattern: (information struct, allocator callbacks [usually nullptr], object).
    VkResult result = vkCreateInstance(&create_info, nullptr, instance);
    return validate_vulkan_result(result, "Failed to create VkInstance.");
}

bool create_window_surface(VkSurfaceKHR* surface, VkInstance* instance, GLFWwindow* window)
{
    /*
    This implementation example creates a custom surface for windows devices.

    VkWin32SurfaceCreateInfoKHR surface_create_info{};

    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hwnd = glfwGetWin32Window(window);
    surface_create_info.hinstance = GetModuleHandle(nullptr); //Get module handle returns the HINSTANCE handle of the current Win32 process.

    if(vkCreateWin32SurfaceKHR(*instance, &surface_create_info, nullptr, surface) != VK_SUCCESS)
    {
        std::cerr << "Failed to create VkSurfaceKHR Win32 object." << std::endl;
        return false;
    }
    */

    //GLFW comes with it's own handler for creating VkSurfaceKHRs with whatever the native platform happens to be.
    if(glfwCreateWindowSurface(*instance, window, nullptr, surface) != VK_SUCCESS)
    {
        std::cerr << "Failed to create VkSurfaceKHR Win32 object." << std::endl;
        return false;
    }

    return true;
}

QueueFamilyProperties get_queue_family_properties(VkPhysicalDevice* device, VkSurfaceKHR* surface)
{
    //Nearly every action or operation in Vulkan needs to be submitted to a queue. Different types of queues originate from different queue famililes.
    //These queue families only allow a subset of commands, for example, one family may only allow graphics commands, one may only allow compute commands, etc.
    
    QueueFamilyProperties properties;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queue_family_count, nullptr);

    if(queue_family_count == 0)
    {
        return properties;
    }

    VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queue_family_count, queue_families);

    //Note, need to come back later and figure out which is the 'best' queue family to use.
    for(uint32_t i = 0; i < queue_family_count; i++)
    {
        //Just checking for queue families which have the graphics bit enabled.
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            properties.has_graphics = true;
            properties.graphics_index = i;
        }

        //Now checking if said queue family has presentation support.
        VkBool32 has_presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(*device, i, *surface, &has_presentation_support);

        if(has_presentation_support)
        {
            properties.has_presentation = true;
            properties.presentation_index = i;
        }
    }

    delete[] queue_families;
    return properties;
}

SwapChainSupportProperties get_swap_chain_properties(VkPhysicalDevice* device, VkSurfaceKHR* surface)
{
    SwapChainSupportProperties properties;

    //Polling the capabilities of the surface indicates what the resolution of the images are in the swap chain, among other properties.

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, *surface, &properties.capabilities);

    //Surface formats indicate details about the format of the images being used in the swap chain, such as color depth.

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &format_count, nullptr);

    if(format_count != 0)
    {
        properties.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(*device, *surface, &format_count, properties.formats.data());
    }

    //A presentation mode determines the conditions required for an image swap to occur within the swap chain.

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*device, *surface, &present_mode_count, nullptr);

    if(present_mode_count != 0)
    {
        properties.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(*device, *surface, &present_mode_count, properties.present_modes.data());
    }

    //We store the capabilities, as well as all possible formats and presentation modes inside of a struct and make a decision on what to choose later.
    return properties;
}

VkSurfaceFormatKHR choose_swap_surface_format(SwapChainSupportProperties properties)
{
    for(VkSurfaceFormatKHR format : properties.formats)
    {
        //Using sRGB if it's available.

        /*
        sRGB is essentially RGB but with a 'curve' on otherwise linear intensities that produces more dark tones compared
        to linear RGB. This is to correct for the human eye, which is able to more easily distinguish darker tones apart from
        each other compared to lighter tones, and reduces banding in environments with darker tones.
        */

        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    //Otherwise we pick the first thing that's available.
    return properties.formats[0];
}

VkPresentModeKHR choose_swap_present_mode(SwapChainSupportProperties properties)
{
    //In this case, we want to prioritize two different presentation modes: FIFO, and mailbox.
    //Mailbox is preferred, but if it's unavailable, FIFO is picked instead. If both are unavailable, then whatever's left gets picked.

    /*
    The following is a quick description of each presentation mode in Vulkan 1.0:

    VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted to the swap chain are presented immediately. This could result in tearing, or
    skipped frames. It is the least desired solution, as it essentially ignores the swap chain outright.

    VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue, where the display takes an image from the front of the queue upon refresh
    and displays it. Images being rendered to are at the back of the queue. FIFO is short for first in, first out. Very similar to
    V-sync in modern games.

    VK_PRESENT_MODE_FIFO_RELAXED_KHR: Similar to FIFO, but if the queue runs out before the next image is fully processed, it still
    gets displayed, which will likely result in some tearing (as opposed to just showing nothing).

    VK_PRESENT_MODE_MAILBOX_KHR: Another FIFO variation. If the queue is full, the images that are already queued are replaced with
    newer ones. This mode avoids tearing and is fast, so it's the one I'm preferring in this demo.
    */

    bool has_fifo = false;
    VkPresentModeKHR fifo;
    for(VkPresentModeKHR present_mode : properties.present_modes)
    {
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
        if(present_mode == VK_PRESENT_MODE_FIFO_KHR && !has_fifo)
        {
            has_fifo = true;
            fifo = present_mode;
        }
    }

    if(has_fifo) return fifo;
    else return properties.present_modes[0];
}

VkExtent2D choose_swap_extent(SwapChainSupportProperties properties, GLFWwindow* window)
{
    VkSurfaceCapabilitiesKHR c = properties.capabilities;
    if(c.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return c.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D extent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, c.minImageExtent.width, c.maxImageExtent.width);
        extent.width = std::clamp(extent.height, c.minImageExtent.height, c.maxImageExtent.height);

        return extent;
    }
}

//Verifying if the physical device in question supports the required extensions.
bool check_device_extension_support(VkPhysicalDevice* physical_device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(*physical_device, nullptr, &extension_count, nullptr);

    VkExtensionProperties* extensions = new VkExtensionProperties[extension_count];
    vkEnumerateDeviceExtensionProperties(*physical_device, nullptr, &extension_count, extensions);

    std::set<std::string> unaccounted_extensions;
    for(uint32_t i = 0; i < APP_REQUIRED_EXTENSIONS_SIZE; i++)
    {
        unaccounted_extensions.insert(required_device_extensions[i]);
    }

    for(uint32_t i = 0; i < extension_count; i++)
    {
        unaccounted_extensions.erase(extensions[i].extensionName);
    }

    return unaccounted_extensions.empty();
}

//Select a physical device (GPU) to use, and store within physical_device.
bool select_physical_device(VkPhysicalDevice* physical_device, VkInstance* instance, VkSurfaceKHR* surface)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(*instance, &device_count, nullptr); //We just get the device count first, to check if there are any devices which support Vulkan.

    if(device_count == 0)
    {
        std::cerr << "No GPUs in the system support Vulkan." << std::endl;
        return false;
    }

    VkPhysicalDevice* devices = new VkPhysicalDevice[device_count];
    vkEnumeratePhysicalDevices(*instance, &device_count, devices); //Now we get the full device properties.

    uint32_t max_score = 0;

    //Rather than just selecting the first GPU I see, I instead opt for giving each GPU a 'score' based on it's properties, and choosing the
    //one with the highest score. Right now, all I care about is whether the GPU in question is discrete or not, and whether the GPU queue
    //family supports graphics commands and contains the required extensions (this is a must). Having the same queue also have presentation
    //commands is a plus.
    for(uint32_t i = 0; i < device_count; i++)
    {
        uint32_t current_score = 1;
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(devices[i], &device_properties);

        current_score += 10 * (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

        QueueFamilyProperties queue_family_properties = get_queue_family_properties(&devices[i], surface);
        if(queue_family_properties.has_presentation) current_score++;
        if(!queue_family_properties.has_graphics || !check_device_extension_support(&devices[i])) current_score = 0;

        if(current_score != 0)
        {
            //Checking to make sure our swap chain supports at least one image format and one presentation mode.
            SwapChainSupportProperties swap_chain_properties = get_swap_chain_properties(&devices[i], surface);
            current_score = (!swap_chain_properties.formats.empty() && !swap_chain_properties.present_modes.empty()) ? current_score : 0;
        }

        if(current_score > max_score)
        {
            max_score = current_score;
            *physical_device = devices[i];
        }
    }

    //If the max_score remains at 0, no GPUs are suitable.
    if(max_score == 0)
    {
        std::cerr << "Could not find any suitable GPUs for the application." << std::endl;
        delete[] devices;
        return false;
    }

    delete[] devices;
    return true;
}

bool create_logical_device(VkDevice* device, VkSurfaceKHR* surface, VkPhysicalDevice* physical_device, VkInstance* instance)
{
    QueueFamilyProperties properties = get_queue_family_properties(physical_device, surface);

    std::set<uint32_t> unique_indices = {properties.graphics_index, properties.presentation_index};
    size_t queues_size = unique_indices.size();

    VkDeviceQueueCreateInfo* queue_create_info = new VkDeviceQueueCreateInfo[queues_size];

    int i = 0;
    float queue_priority = 1;
    for(uint32_t index : unique_indices)
    {
        queue_create_info[i] = VkDeviceQueueCreateInfo{};
        queue_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[i].queueFamilyIndex = index;
        queue_create_info[i].queueCount = 1; //You should only need a small number of queues for each queue family, no more than 1 in most cases.

        //Each queue should have an associated queue priority value, stored in an array of floats. In this case, we're only using 1 queue.
        queue_create_info[i].pQueuePriorities = &queue_priority;
        i++;
    }

    VkPhysicalDeviceFeatures physical_device_features{}; //We'll come back to this later.

    VkDeviceCreateInfo device_create_info{};

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.queueCreateInfoCount = queues_size;
    device_create_info.pEnabledFeatures = &physical_device_features;

    device_create_info.enabledExtensionCount = APP_REQUIRED_EXTENSIONS_SIZE;
    device_create_info.ppEnabledExtensionNames = required_device_extensions;

    if(enable_validation_layers)
    {
        device_create_info.enabledLayerCount = APP_VALIDATION_LAYERS_SIZE;
        device_create_info.ppEnabledLayerNames = validation_layers;
    }
    else device_create_info.enabledLayerCount = 0;

    if(vkCreateDevice(*physical_device, &device_create_info, nullptr, device) != VK_SUCCESS)
    {
        std::cerr << "Vulkan failed to create logical device." << std::endl;
        delete[] queue_create_info;
        return false;
    }

    delete[] queue_create_info;
    return true;
}

bool create_swap_chain(VkSwapchainKHR* swapchain, VkDevice* device, VkPhysicalDevice* physical_device, VkSurfaceKHR* surface, GLFWwindow* window, VkFormat* swapchain_format, VkExtent2D* swapchain_extent)
{
    SwapChainSupportProperties swapchain_properties = get_swap_chain_properties(physical_device, surface);

    VkSurfaceFormatKHR format = choose_swap_surface_format(swapchain_properties);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_properties);
    VkExtent2D extent = choose_swap_extent(swapchain_properties, window);

    *swapchain_format = format.format;
    *swapchain_extent = extent;

    //We need to decide how many images are present in the swap chain. This can be done by getting the minimum number of required images, and then adding 1
    //to make sure we give the driver a little extra time to breathe.

    uint32_t image_count = swapchain_properties.capabilities.minImageCount + 1;
    if(swapchain_properties.capabilities.maxImageCount != 0 && image_count > swapchain_properties.capabilities.maxImageCount)
    {
        //maxImageCount being 0 means there's no maximum limit on how many images can be in the swap chain.
        image_count = swapchain_properties.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = *surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = format.format;
    swapchain_create_info.imageColorSpace = format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1; //Unless you're developing some kind of stereoscopic 3D app, this value is always 1.

    //This refers to how images are going to be used. Since we're rendering directly to them, we use VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.
    //Other uses include post-processing, which might be done using VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyProperties queue_properties = get_queue_family_properties(physical_device, surface);
    uint32_t queue_indices[] = {queue_properties.graphics_index, queue_properties.presentation_index};

    if(queue_indices[0] != queue_indices[1])
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchain_create_info.preTransform = swapchain_properties.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE; //We don't care about the color of pixels which are obscured. This provides a performance boost.
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE; //Sometimes, swap chains become invalid (like when you resize a window). We'll handle this later.

    if(vkCreateSwapchainKHR(*device, &swapchain_create_info, nullptr, swapchain) != VK_SUCCESS)
    {
        std::cerr << "Failed to create swapchain." << std::endl;
        return false;
    }

    return true;
}

bool create_shader_module(char* bytecode, size_t bytecode_size, VkShaderModule* shader_module, VkDevice* device)
{
    //This function should be pretty self-explanatory. We're creating a VkShaderModule object based on the shader bytecode.

    VkShaderModuleCreateInfo shader_module_create_info{};

    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = bytecode_size;
    shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(bytecode);

    if(vkCreateShaderModule(*device, &shader_module_create_info, nullptr, shader_module) != VK_SUCCESS)
    {
        std::cerr << "Failed to create shader module." << std::endl;
        return false;
    }

    return true;
}

bool create_render_pass(VkRenderPass* render_pass, VkDevice* device, VkFormat* format)
{
    VkAttachmentDescription attachment_color{};
    attachment_color.format = *format;
    attachment_color.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling.
    attachment_color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //Clearing all values to a constant when rendering starts.
    attachment_color.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //Storing all values in memory to be read later when rendering finishes.
    attachment_color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //What layout will the image have before the render pass begins? We don't care.
    attachment_color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //Image will be presented in the swap chain.

    VkAttachmentReference attachment_ref_color{};
    attachment_ref_color.attachment = 0;
    attachment_ref_color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_ref_color;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info{};

    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &attachment_color;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if(vkCreateRenderPass(*device, &render_pass_create_info, nullptr, render_pass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass." << std::endl;
        return false;
    }

    return true;
}

bool create_vulkan_pipeline(VkPipeline* pipeline, VkPipelineLayout* pipeline_layout, VkDevice* device, VkExtent2D* extent, VkRenderPass* render_pass)
{
    char* main_vs;
    char* main_fs;

    size_t vs_size, fs_size;

    //Shaders aren't JIT-compiled in Vulkan like they are in OpenGL. Instead, they're AOT-compiled, so rather than reading GLSL source code, we must
    //read SPV bytecode. The Vulkan SDK fortunately comes with a GLSL-to-SPV compiler.

    if(!read_spv_file(&main_vs, &vs_size, "../src/shaders/main_vs.spv")) return false;
    if(!read_spv_file(&main_fs, &fs_size, "../src/shaders/main_fs.spv")) return false;

    VkShaderModule vs_shader_module;
    VkShaderModule fs_shader_module;

    std::vector<VkShaderModule> shader_modules;

    if(!create_shader_module(main_vs, vs_size, &vs_shader_module, device)) return false;
    if(!create_shader_module(main_fs, fs_size, &fs_shader_module, device)) return false;

    shader_modules.push_back(vs_shader_module);
    shader_modules.push_back(fs_shader_module);

    VkPipelineShaderStageCreateInfo vertex_stage_create_info{};

    vertex_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT; //Type of shader.
    vertex_stage_create_info.module = vs_shader_module;
    vertex_stage_create_info.pName = "main"; //Shader entry point.

    VkPipelineShaderStageCreateInfo fragment_stage_create_info{};

    fragment_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage_create_info.module = fs_shader_module;
    fragment_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_stage_create_info, fragment_stage_create_info};

    //From here, it's time to describe how each step of the pipeline process is going to work.

    //Pipeline dynamic states store the states that can change during runtime without switching to a different rendering pipeline.
    //This can include the size of the viewport, as well as blend constants.

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{};

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.dynamicStateCount = 2;
    pipeline_dynamic_state_create_info.pDynamicStates = dynamic_states;

    //The vertex input state describes how the vertex data for the application will be passed into the vertex shader.

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{};

    //We're baking the vertex data directly into the shader for now, so we don't need to worry about this much.

    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;

    //The input assembly state literally describes how the vertex data will be 'assembled,' what kind of geometry will result
    //from it, and whether primitive restart is enabled.

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};

    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    //The viewport determines where on the framebuffer the pixels will be drawn. In most cases, it will be the current size of the framebuffer.

    VkViewport viewport{};

    viewport.x = 0;
    viewport.y = 0;
    viewport.width = extent->width;
    viewport.height = extent->height;

    //See the rasterization state creation for info on what these two values mean.
    viewport.minDepth = 0; 
    viewport.maxDepth = 1;

    //Scissor rectangles will be used to define where pixels will actually be stored. Where viewports are transformations from the image to the
    //framebuffer, scissors function more as filters.

    //Once again though, we just want to draw to the entire framebuffer.

    VkRect2D scissor{};

    scissor.offset = {0, 0}; //This is a VkOffset2D object.
    scissor.extent = *extent;

    //Now we can describe the viewport state.

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{};

    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = &viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = &scissor;

    //The rasterization state describes how vertex geometry is rasterized into fragments, which are passed into the fragment shader.

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{};

    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE; //Clamps depth values to the ones specified in the viewport, as opposed to culling them entirely.
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE; //If this is enabled, data is never passed through the rasterizer stage. This essentially disables output.

    /*
    The polygonModes that are available in Vulkan 1.0 are as follows:
    
    VK_POLYGON_MODE_FILL: Fills the area within the polygon with fragments.
    VK_POLYGON_MODE_LINE: Only fills polygon edges, creating lines.
    VK_POLYGON_MODE_POINT: Only draw polygon vertices as points.

    Using any mode other than fill requires enabling a GPU feature in the logical device creation step.
    */

    pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.lineWidth = 1; //Any other value requires enabling a GPU feature.
    pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT; //Determines how face culling will work.
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE; //Specifies the vertex order that determines what a front-facing face is.

    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE; //Depth biasing adds a constant value or some sort of curve to depth values. We aren't using this for now.

    //The multisample state specifies how multisampling is going to work, which is one way to implement anti-aliasing.

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{};

    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //The color blend attachment state determines how colors in the new image will be blended with the colors in the current framebuffer. This is
    //commonly used for alpha blending, but we don't need to worry about that right now.

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{};

    pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;
    pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{};

    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

    //Now we can finally begin working on the pipeline, starting with the pipeline layout.

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if(vkCreatePipelineLayout(*device, &pipeline_layout_create_info, nullptr, pipeline_layout) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline layout." << std::endl;
        return false;
    }

    //The graphics pipeline creation struct will store all of the other structs defined above.

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};

    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = shader_stage_create_infos;

    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;

    graphics_pipeline_create_info.layout = *pipeline_layout;
    graphics_pipeline_create_info.renderPass = *render_pass;
    graphics_pipeline_create_info.subpass = 0;

    if(vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, pipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create graphics pipeline." << std::endl;
        return false;
    }

    vkDestroyShaderModule(*device, shader_modules[0], nullptr);
    vkDestroyShaderModule(*device, shader_modules[1], nullptr);

    return true;
}

bool get_queue_handles(VkDevice* device, VkSurfaceKHR* surface, VkPhysicalDevice* physical_device, VkQueue* queue_graphics, VkQueue* queue_presentation)
{
    QueueFamilyProperties properties = get_queue_family_properties(physical_device, surface);

    vkGetDeviceQueue(*device, properties.graphics_index, 0, queue_graphics);
    vkGetDeviceQueue(*device, properties.presentation_index, 0, queue_presentation);

    return true;
}

bool get_swap_chain_image_handles(std::vector<VkImage>* images, VkDevice* device, VkSwapchainKHR* swapchain)
{
    uint32_t image_count;
    vkGetSwapchainImagesKHR(*device, *swapchain, &image_count, nullptr);

    images->resize(image_count);
    vkGetSwapchainImagesKHR(*device, *swapchain, &image_count, images->data());

    return true;
}

bool get_swap_chain_image_views(std::vector<VkImageView>* image_views, std::vector<VkImage>* images, VkFormat* image_format, VkDevice* device)
{
    //An image view describes how to access an image, and which part to access.

    image_views->resize(images->size());

    for(uint32_t i = 0; i < images->size(); i++)
    {
        VkImageViewCreateInfo image_view_create_info{};

        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = (*images)[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; //This field specifies how the data should be interpreted.
        image_view_create_info.format = *image_format;

        //SWIZZLE_IDENTITY sticks to the default component mapping, ensuring no transformations occur to the colors in our image.
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        //The subresourceRange field describes the image's purpose, and how it should be accessed.
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(*device, &image_view_create_info, nullptr, image_views->data() + i) != VK_SUCCESS)
        {
            std::cerr << "Failed to create image view." << std::endl;
            return false;
        }
    }

    return true;
}

bool get_swap_chain_framebuffers(std::vector<VkFramebuffer>* framebuffers, std::vector<VkImageView>* image_views, VkRenderPass* render_pass, VkExtent2D* extent, VkDevice* device)
{
    framebuffers->resize(image_views->size());

    for(uint32_t i = 0; i < framebuffers->size(); i++)
    {
        VkImageView attachment[] = {(*image_views)[i]}; 
        VkFramebufferCreateInfo framebuffer_create_info{};

        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = *render_pass;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = attachment;
        framebuffer_create_info.width = extent->width;
        framebuffer_create_info.height = extent->height;
        framebuffer_create_info.layers = 1;

        if(vkCreateFramebuffer(*device, &framebuffer_create_info, nullptr, framebuffers->data() + i) != VK_SUCCESS)
        {
            std::cerr << "Failed to create framebuffer (" << i << ")." << std::endl;
            return false;
        }
    }

    return true;
}

bool create_command_pool(VkCommandPool* command_pool, VkDevice* device, VkPhysicalDevice* physical_device, VkSurfaceKHR* surface)
{
    QueueFamilyProperties properties = get_queue_family_properties(physical_device, surface);

    VkCommandPoolCreateInfo command_pool_create_info{};

    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = properties.graphics_index;

    if(vkCreateCommandPool(*device, &command_pool_create_info, nullptr, command_pool) != VK_SUCCESS)
    {
        std::cerr << "Failed to create command pool." << std::endl;
        return false;
    }

    return true;
}

bool allocate_command_buffer(std::vector<VkCommandBuffer>* command_buffers, VkCommandPool* command_pool, VkDevice* device)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};

    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = *command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = command_buffers->size();

    if(vkAllocateCommandBuffers(*device, &command_buffer_allocate_info, command_buffers->data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate command buffer." << std::endl;
        return false;
    }

    return true;
}

bool record_command_buffer(VkCommandBuffer* command_buffer, VkRenderPass* render_pass, VkPipeline* pipeline, std::vector<VkFramebuffer>* framebuffers, VkExtent2D* extent, uint32_t image_index)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if(vkBeginCommandBuffer(*command_buffer, &command_buffer_begin_info) != VK_SUCCESS)
    {
        std::cerr << "Failed to begin recording command buffer." << std::endl;
        return false;
    }

    VkClearValue clear_color = {{{0, 0, 0, 1}}};

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = *render_pass;
    render_pass_begin_info.framebuffer = (*framebuffers)[image_index];
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = *extent;
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(*command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = extent->width;
    viewport.height = extent->height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    vkCmdSetViewport(*command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = *extent;

    vkCmdSetScissor(*command_buffer, 0, 1, &scissor);

    vkCmdDraw(*command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(*command_buffer);

    if(vkEndCommandBuffer(*command_buffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to finish recording command buffer." << std::endl;
        return false;
    }

    return true;
}

bool draw_frame(std::vector<VkCommandBuffer>* command_buffers, VkRenderPass* render_pass, VkPipeline* pipeline, std::vector<VkFramebuffer>* framebuffers, VkExtent2D* extent,
std::vector<VkSemaphore>* image_available_semaphores, std::vector<VkSemaphore>* signal_semaphores, std::vector<VkFence>* in_flight_fences, VkSwapchainKHR* swapchain,
VkQueue* queue_graphics, VkQueue* queue_presentation, VkDevice* device, unsigned int current_frame)
{
    vkWaitForFences(*device, 1, in_flight_fences->data() + current_frame, VK_TRUE, UINT64_MAX);
    vkResetFences(*device, 1, in_flight_fences->data() + current_frame);

    uint32_t image_index;
    vkAcquireNextImageKHR(*device, *swapchain, UINT64_MAX, (*image_available_semaphores)[current_frame], VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer((*command_buffers)[current_frame], 0);
    record_command_buffer(command_buffers->data() + current_frame, render_pass, pipeline, framebuffers, extent, image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore* wait_sem = image_available_semaphores->data() + current_frame;
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_sem;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = command_buffers->data() + current_frame;

    VkSemaphore* sig_sem = signal_semaphores->data() + current_frame;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = sig_sem;

    if(vkQueueSubmit(*queue_graphics, 1, &submit_info, (*in_flight_fences)[current_frame]) != VK_SUCCESS)
    {
        std::cerr << "Failed to submit draw frame to graphics queue." << std::endl;
        return false;
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = sig_sem;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(*queue_presentation, &present_info);

    return true;
}

void clean_sync_objects(std::vector<VkSemaphore>* image_available_semaphores, std::vector<VkSemaphore>* signal_semaphores, std::vector<VkFence>* in_flight_fences, VkDevice* device)
{
    vkDeviceWaitIdle(*device);

    for(uint32_t i = 0; i < image_available_semaphores->size(); i++)
        vkDestroySemaphore(*device, (*image_available_semaphores)[i], nullptr);

    for(uint32_t i = 0; i < signal_semaphores->size(); i++)
        vkDestroySemaphore(*device, (*signal_semaphores)[i], nullptr);
    
    for(uint32_t i = 0; i < in_flight_fences->size(); i++)
        vkDestroyFence(*device, (*in_flight_fences)[i], nullptr);
}

//Destroy all Vulkan objects after the window closes.
void clean(VkInstance* instance, VkDebugUtilsMessengerEXT* messenger, VkSurfaceKHR* surface, VkDevice* device, VkSwapchainKHR* swapchain, std::vector<VkImageView>* image_views,
VkRenderPass* render_pass, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline, std::vector<VkFramebuffer>* framebuffers, VkCommandPool* command_pool)
{
    vkDestroyCommandPool(*device, *command_pool, nullptr);
    for(uint32_t i = 0; i < framebuffers->size(); i++)
    {
        vkDestroyFramebuffer(*device, (*framebuffers)[i], nullptr);
    }
    vkDestroyPipeline(*device, *pipeline, nullptr);
    vkDestroyPipelineLayout(*device, *pipeline_layout, nullptr);
    vkDestroyRenderPass(*device, *render_pass, nullptr);
    for(uint32_t i = 0; i < image_views->size(); i++)
    {
        vkDestroyImageView(*device, (*image_views)[i], nullptr);
    }
    vkDestroySwapchainKHR(*device, *swapchain, nullptr);
    vkDestroyDevice(*device, nullptr);
    vkDestroySurfaceKHR(*instance, *surface, nullptr);
    if(enable_validation_layers) DestroyDebugUtilsMessengerEXT(*instance, *messenger, nullptr);
    vkDestroyInstance(*instance, nullptr);
}

int main()
{
    //Creating the window, and initializing GLFW context.
    GLFWwindow* window = init_window();

    /*
    The initialzation of Vulkan and the steps required to set up the triangle drawing application are as follows:

    Step 1: Create a VkInstance, to describe the application and any API extensions you might be using. Also, set up any validation
    layers if necessary.

    Step 2: Create a window surface to render to. For the sake of this demo, GLFW will be used for window and input handling.

    Step 3: Select a VkPhysicalDevice (GPU) to use for graphical operations.

    Step 4: Create a VkDevice to describe specifically what features of the VkPhysicalDevice you'll be using for the application,
    such as multi-viewport rendering, 64-bit floating point precision, etc.

    Step 5: Specify which queue families are being used. Vulkan operations are often performed asynchronously by submitting them
    to VkQueue. Different queues are allocated from queue families, which can be used seperately for graphics, computation, memory
    transfer, etc.

    Step 6: Create the swap chain. The swap chain is a collection of render targets whose basic purpose is to ensure that only
    complete images are displayed on screen. Every time a frame needs to be rendered, the swap chain needs to provide an image to
    be rendered. When the frame is finished, it needs to be returned to the swap chain, and eventually presented at some point.

    Step 7: Drawing an image from the swap chain requires wrapping it in a VkImageView, which references a specific part of the
    image to be used, and then wrapping that in a VkFramebuffer, which defines how the image view is going to be used in context
    of the application (color, depth buffer, stencil, etc.).

    Step 8: Vulkan also uses render passes to describe how images will be used during rendering operations. A render pass
    essentially acts as a slot for different types of framebuffers to slide into.

    Step 9: Create a VkPipeline object to describe the state of the graphics card (such as the viewport size, depth buffer
    operation) and the programmable state using VkShaderModule objects. VkShaderModule objects are created from shader bytecode.
    Virtually all of the shader configuration needs to be set in advance, so multiple VkPipelines will need to be used if switching
    to a different shader or changing the vertex layout. Some basic information, like viewport size, can be changed dynamically,
    though. The tradeoff is that Vulkan uses ahead-of-time compilation rather than just-in-time compilation, so performance is more
    predictable and easier to optimize.

    Step 10: Once the graphics pipeline is complete, we can finally create the framebuffers for each image in the swap chain.

    Step 11: Queue operations need to be recorded into a VkCommandBuffer before being submitted to a queue. These command buffers
    are allocated from a VkCommandPool, which is associated with a specific queue family. For drawing a triangle, these are the
    commands that will need to be supported:

        • Begin render pass.
        • Bind graphics pipeline.
        • Draw 3 vertices.
        • End render pass.

    A command buffer must be recorded for each possible image in the swap chain and we need to select the right one to draw each frame.

    Step 12: Establish the draw loop. Acquire the next available frame from the swap chain, select the appropriate command buffer for
    that image and execute it with VkQueueSubmit. Then, return the image to the swap chain for eventual presentation to the screen.
    */

    //The majority of information you'll be providing to Vulkan comes in the form of structs that you specify and send to Vulkan's APIs.

    //This little bit of test code is a simple way to make sure you can compile and test Vulkan code properly.

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::cout << extension_count << " extensions supported\n";

    //Step 1: Create the VkInstance, and setting up validation layers.
    VkInstance instance;
    if(!create_vulkan_instance(&instance)) return 1;

    VkDebugUtilsMessengerEXT debug_messenger;
    if(enable_validation_layers && !setup_debug_messenger(&instance, &debug_messenger)) return 1;

    //Step 2: Define the window surface using VkSurfaceKHR. This can influence which physical GPU to use.
    VkSurfaceKHR surface;
    if(!create_window_surface(&surface, &instance, window));

    //Step 3: Selecting a physical GPU. This also covers most of step 4.
    VkPhysicalDevice physical_device;
    if(!select_physical_device(&physical_device, &instance, &surface)) return 1;

    //Step 4: Creating the logical device, and getting the necessary queue handles.
    VkDevice device;
    if(!create_logical_device(&device, &surface, &physical_device, &instance)) return 1;

    //Step 5: Get the required queue handles.
    VkQueue queue_graphics, queue_presentation;
    if(!get_queue_handles(&device, &surface, &physical_device, &queue_graphics, &queue_presentation)) return 1;

    //Step 6: Create the swapchain, and poll the swapchain format, extent (resolution), and images.
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    if(!create_swap_chain(&swapchain, &device, &physical_device, &surface, window, &format, &extent)) return 1;
    
    std::vector<VkImage> images;
    if(!get_swap_chain_image_handles(&images, &device, &swapchain)) return 1;

    //Step 7: Creating the image views for each image in the swap cahin.
    std::vector<VkImageView> image_views;
    if(!get_swap_chain_image_views(&image_views, &images, &format, &device)) return 1;

    //Step 8: Creating a render pass object for storing information about the framebuffer's use of images.
    VkRenderPass render_pass;
    if(!create_render_pass(&render_pass, &device, &format)) return 1;

    //Step 9: Reading the shader bytecode, and preparing the VkPipelineLayout and VkPipeline.
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    if(!create_vulkan_pipeline(&pipeline, &pipeline_layout, &device, &extent, &render_pass)) return 1; 

    //Step 10: Creating framebuffers for each image in the swap chain.
    std::vector<VkFramebuffer> framebuffers;
    if(!get_swap_chain_framebuffers(&framebuffers, &image_views, &render_pass, &extent, &device)) return 1;

    //Step 11: Creating command pools and command buffers for drawing.
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers_draw;
    command_buffers_draw.resize(MAX_FRAMES_IN_FLIGHT);
    
    if(!create_command_pool(&command_pool, &device, &physical_device, &surface)) return 1;
    if(!allocate_command_buffer(&command_buffers_draw, &command_pool, &device)) return 1;

    //Step 12: Setting up synchronization objects and drawing.
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> signal_semaphores;

    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    signal_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

    std::vector<VkFence> in_flight_fences;

    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    if(!create_semaphores(&image_available_semaphores, &device)) return 1;
    if(!create_semaphores(&signal_semaphores, &device)) return 1;

    if(!create_fences(&in_flight_fences, &device)) return 1;

    uint32_t current_frame = 0;

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if(!draw_frame(&command_buffers_draw, &render_pass, &pipeline, &framebuffers, &extent, &image_available_semaphores, &signal_semaphores, &in_flight_fences,
                       &swapchain, &queue_graphics, &queue_presentation, &device, current_frame)) return 1;

        current_frame++;
        current_frame %= MAX_FRAMES_IN_FLIGHT;
    }

    clean_sync_objects(&image_available_semaphores, &signal_semaphores, &in_flight_fences, &device);
    clean(&instance, &debug_messenger, &surface, &device, &swapchain, &image_views, &render_pass, &pipeline_layout, &pipeline, &framebuffers, &command_pool);

    //Destroying the window and destroying the GLFW context.
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}