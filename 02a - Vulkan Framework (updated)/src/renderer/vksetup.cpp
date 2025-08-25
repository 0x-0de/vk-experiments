#include "vksetup.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

#include "swapchain.h"

#include "GLFW/glfw3.h"

#define VALIDATION_LAYER_COUNT 1

static const char* validation_layers[VALIDATION_LAYER_COUNT] =
{
	"VK_LAYER_KHRONOS_validation"
};

#define INSTANCE_EXTENSION_COUNT 1
static const char* inst_extensions[INSTANCE_EXTENSION_COUNT] =
{
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

#define DEVICE_EXTENSION_COUNT 1
static const char* device_extensions[DEVICE_EXTENSION_COUNT] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkDebugUtilsMessengerEXT vl_debug_messenger;

static VkInstance vk_instance;

static VkPhysicalDevice vk_physical_device;
static VkDevice vk_device;

static VkSurfaceKHR vk_surface;

static VKAPI_ATTR VkBool32 VKAPI_CALL vl_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
	switch(message_severity)
	{
		default:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			std::cerr << "[VK|VAL] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			std::cerr << "[VK|VAL|INF] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			std::cerr << "[VK|VAL|WRN] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			std::cerr << "[VK|VAL|ERR] ";
			break;
	}
    std::cerr << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

const std::string get_result_code_str(VkResult error_code)
{
	switch(error_code)
	{
		case 0:
			return "VK_SUCCESS\nOperation completed successfully.";
		case 1:
			return "VK_NOT_READY\nA fence or query has not yet completed.";
		case 2:
			return "VK_TIMEOUT\nA wait operation has not completed in the specified time.";
		case 3:
			return "VK_EVENT_SET\nAn event is signaled.";
		case 4:
			return "VK_EVENT_RESET\nAn event is unsignaled.";
		case 5:
			return "VK_INCOMPLETE\nA return array was too small for the result.";
		case -1:
			return "VK_ERROR_OUT_OF_HOST_MEMORY\nA host memory allocation operation has failed.";
		case -2:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY\nA device memory allocation operation has failed.";
		case -3:
			return "VK_ERROR_INITIALIZATION_FAILED\nInitialization of an object could not be completed for implementation-specific reasons.";
		case -4:
			return "VK_ERROR_DEVICE_LOST\nThe logical or physical device has been lost.";
		case -5:
			return "VK_ERROR_MEMORY_MAP_FAILED\nMapping of a memory object has failed.";
		case -6:
			return "VK_ERROR_LAYER_NOT_PRESENT\nA requested layer is not present or could not be loaded.";
		case -7:
			return "VK_ERROR_EXTENSION_NOT_PRESENT\nA requested extension is not supported.";
		case -8:
			return "VK_ERROR_FEATURE_NOT_PRESENT\nA requested feature is not supported.";
		case -9:
			return "VK_ERROR_INCOMPATIBLE_DRIVER\nThe requested version of Vulkan is not supported by the driver or is otherwise incompatible implementation-specific reasons.";
		case -10:
			return "VK_ERROR_TOO_MANY_OBJECTS\nToo many objects of the type have already been created.";
		case -11:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED\nA requested format is not supported on this device.";
		case -12:
			return "VK_ERROR_FRAGMENTED_POOL\nA pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
		case -13:
			return "VK_ERROR_UNKNOWN\nAn unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
		default:
			return "Unknown error\nThis error code [" + std::to_string(error_code) + "] isn't recognized by Vulkan or this function.";
	}
}

void report_vulkan_error(const char* msg, VkResult error_code)
{
	std::cerr << "[VK|ERR] " << msg << std::endl;
	std::cerr << "Error code: " << get_result_code_str(error_code) << std::endl;
}

//Deinitializes all basic Vulkan objects (such as the instance and logical device).
void deinit_vulkan_application()
{
	vkDestroyDevice(vk_device, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan device." << std::endl;
#endif
	vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Destroyed Vulkan surface handler." << std::endl;
#endif
#ifndef NDEBUG
	auto vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");
	if(vkDestroyDebugUtilsMessenger != nullptr) vkDestroyDebugUtilsMessenger(vk_instance, vl_debug_messenger, nullptr);
	#ifdef DEBUG_PRINT_SUCCESS
		std::cout << "[VK|INF] Freed Vulkan validation layers." << std::endl;
	#endif
#endif

	vkDestroyInstance(vk_instance, nullptr);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Deinitialized Vulkan." << std::endl;
#endif
}

//Obtains a (heap-allocated) list of required instance extensions for Vulkan.
//Basically assembles the list from GLFW's glfwGetRequiredInstanceExtensions function and inst_extensions.
bool get_required_extensions(const char*** extensions, uint32_t* extension_count)
{
	//Vulkan is platform-agostic, meaning it requires certain extensions to interface with the Windows API, and GLFW.
	//GLFW provides a handy helper function which lists all of the extensions the current GLFW context requires.
	
	uint32_t glfw_extension_count;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	*extension_count = INSTANCE_EXTENSION_COUNT + glfw_extension_count;
	
	const char** total_extensions = new const char*[*extension_count];
	
	for(uint32_t i = 0; i < glfw_extension_count; i++)
		total_extensions[i] = glfw_extensions[i];
	for(uint32_t i = 0; i < INSTANCE_EXTENSION_COUNT; i++)
		total_extensions[glfw_extension_count + i] = inst_extensions[i];
		
	*extensions = total_extensions;
	return true;
}

//Similar to get_required_extensions, but gets all required validation layers instead.
bool get_required_validation_layers(const char*** layers, uint32_t* layer_count)
{
#ifdef NDEBUG
	*layer_count = 0;
	*layers = nullptr;
#else
	*layer_count = VALIDATION_LAYER_COUNT;
	const char** total_validation_layers = new const char*[*layer_count];
	
	for(uint32_t i = 0; i < VALIDATION_LAYER_COUNT; i++)
	{
		total_validation_layers[i] = validation_layers[i];
	}
	
	*layers = total_validation_layers;
#endif
	return true;
}

void print_physical_device_queue_families(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties device_props;
	vkGetPhysicalDeviceProperties(device, &device_props);
	
	std::cout << "[VK|INF] Queue families for " << device_props.deviceName << std::endl;
	
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	
	VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	
	for(uint32_t i = 0; i < queue_family_count; i++)
	{
		std::cout << "\tQueue family: " << i << ":" << std::endl;
		std::cout << "\t\tQueue count: " << queue_families[i].queueCount << std::endl;
		std::cout << "\t\tQueue flags: | ";
		if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			std::cout << "Graphics | ";
		if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			std::cout << "Compute | ";
		if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			std::cout << "Transfer | ";
		if(queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			std::cout << "Sparse Binding | ";
		if(queue_families[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
			std::cout << "Protected | ";
		if(queue_families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
			std::cout << "Video Decode | ";
		if(queue_families[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
			std::cout << "Video Encode | ";
		if(queue_families[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
			std::cout << "Optical Flow | ";
		//if(queue_families[i].queueFlags & VK_QUEUE_DATA_GRAPH_BIT_ARM)
			//std::cout << "Data Graph | ";
		std::cout << std::endl;
	}
	
	delete[] queue_families;
}

//
queue_family find_physical_device_queue_families(VkPhysicalDevice device)
{
	queue_family qf;
	
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	
	VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	
	for(uint32_t i = 0; i < queue_family_count; i++)
	{
		if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			qf.queue_index_graphics = i;
		}
		else if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			qf.queue_index_transfer = i;
		}
		
		VkBool32 supports_presentation;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk_surface, &supports_presentation);

		if(supports_presentation) qf.queue_index_present = i;
	}
	
	delete[] queue_families;
	return qf;
}

constexpr VkDebugUtilsMessengerCreateInfoEXT get_vk_debugutils_messenger_info()
{
	VkDebugUtilsMessengerCreateInfoEXT info{};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	info.pfnUserCallback = vl_debug_callback;
	
	return info;
}

bool setup_validation_layers()
{
	VkDebugUtilsMessengerCreateInfoEXT info = get_vk_debugutils_messenger_info();
	
	//Extension functions require you to get the address of the function, and then call it as a function pointer.
	auto vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
	VkResult r = vkCreateDebugUtilsMessenger(vk_instance, &info, nullptr, &vl_debug_messenger);
	VERIFY(r, "Failed to set up Vulkan validation layer messenger.")
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Set up the Vulkan debug messenger." << std::endl;
#endif
	return true;
}

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

bool select_physical_device()
{
	uint32_t device_count;
	vkEnumeratePhysicalDevices(vk_instance, &device_count, 0);
	
	if(device_count == 0)
	{
		std::cerr << "[VK|ERR] Unable to locate any physical devices that support Vulkan." << std::endl;
		return false;
	}
	
	VkPhysicalDevice* available_devices = new VkPhysicalDevice[device_count];
	vkEnumeratePhysicalDevices(vk_instance, &device_count, available_devices);
	
	uint32_t max_score = 0;
	uint32_t current_selection = 0;
	
	for(uint32_t i = 0; i < device_count; i++)
	{
		VkPhysicalDeviceProperties device_props;
		VkPhysicalDeviceFeatures device_feats;
		
		vkGetPhysicalDeviceProperties(available_devices[i], &device_props);
		vkGetPhysicalDeviceFeatures(available_devices[i], &device_feats);
		
		uint32_t score = 0;
		
		if(device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 10;
		else if(device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 1;
		
		queue_family qf = find_physical_device_queue_families(available_devices[i]);
		if(!qf.queue_index_graphics.has_value() || !qf.queue_index_present.has_value() || !check_device_extension_support(available_devices[i]) || !device_feats.samplerAnisotropy)
			score = 0;
		
		swapchain_properties sc_props = query_swapchain_properties(available_devices[i]);
		if(sc_props.formats.size() == 0 || sc_props.presentation_modes.size() == 0) score = 0;
		
		if(score > max_score)
		{
			current_selection = i;
			max_score = score;
		}
	}
	
	if(max_score == 0)
	{
		std::cerr << "[VK|ERR] Unable to find a physical device that supports the Vulkan application." << std::endl;
		delete[] available_devices;
		return false;
	}
	
	vk_physical_device = available_devices[current_selection];
	delete[] available_devices;	
#ifdef DEBUG_PRINT_SUCCESS
	VkPhysicalDeviceProperties device_props;
	vkGetPhysicalDeviceProperties(vk_physical_device, &device_props);
	std::cout << "[VK|INF] Selected physical GPU for Vulkan commands: " << device_props.deviceName << std::endl;
#endif
	return true;
}

bool init_vulkan_device()
{
	if(!select_physical_device()) return false;
	
	print_physical_device_queue_families(vk_physical_device);
	queue_family qf = find_physical_device_queue_families(vk_physical_device);
	
	std::set<uint32_t> queue_indices = {qf.queue_index_graphics.value(), qf.queue_index_present.value()};
	VkDeviceQueueCreateInfo* info_queues = new VkDeviceQueueCreateInfo[queue_indices.size()];

	float queue_priority = 1.f;

	uint32_t i = 0;
	for(uint32_t index : queue_indices)
	{
		info_queues[i] = {};
		info_queues[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info_queues[i].queueFamilyIndex = index;
		info_queues[i].queueCount = 1;
		info_queues[i].pQueuePriorities = &queue_priority;
		i++;
	}
	
	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;
	
	VkDeviceCreateInfo info_device{};
	info_device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info_device.queueCreateInfoCount = queue_indices.size();
	info_device.pQueueCreateInfos = info_queues;
	info_device.pEnabledFeatures = &device_features;
	info_device.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
	info_device.ppEnabledExtensionNames = device_extensions;
#ifndef NDEBUG
	info_device.enabledLayerCount = VALIDATION_LAYER_COUNT;
	info_device.ppEnabledLayerNames = validation_layers;
#else
	info_device.enabledLayerCount = 0;
#endif

	VkResult r = vkCreateDevice(vk_physical_device, &info_device, nullptr, &vk_device);
	VERIFY(r, "Failed to create Vulkan device.")
	
	delete[] info_queues;
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created Vulkan device." << std::endl;
#endif
	return true;
}

bool create_window_surface(GLFWwindow* window)
{
	VkResult r = glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface);
	VERIFY(r, "Failed to create Vulkan window surface.")
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created Vulkan window surface with GLFW." << std::endl;
#endif
	return true;
}

bool init_vulkan_application(GLFWwindow* window)
{
	//Data sent to the driver, allows the driver to make optimizations based on what version of Vulkan and what engine you're using.
	//This structure is technically optional, but may prove useful regardless.
	VkApplicationInfo info_app{};
	info_app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info_app.pApplicationName = "Vulkan Test";
	info_app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	info_app.pEngineName = "No Engine";
	info_app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	info_app.apiVersion = VK_API_VERSION_1_0;
	
	const char** extensions;
	uint32_t extension_count;
	if(!get_required_extensions(&extensions, &extension_count)) return false;
		
	const char** layers;
	uint32_t layer_count = 0;
	if(!get_required_validation_layers(&layers, &layer_count)) return false;
	
#ifndef NDEBUG
	//On the edge case that something with vkCreateInstance fails, we can pass a debug utils messenger struct to pNext to catch the problem.
	VkDebugUtilsMessengerCreateInfoEXT info_debug_instance = get_vk_debugutils_messenger_info();
#endif	

	VkInstanceCreateInfo info_crt_instance{};
	info_crt_instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info_crt_instance.pApplicationInfo = &info_app;
	info_crt_instance.enabledExtensionCount = extension_count;
	info_crt_instance.ppEnabledExtensionNames = extensions;
	info_crt_instance.enabledLayerCount = layer_count;
	info_crt_instance.ppEnabledLayerNames = layers;	
#ifndef NDEBUG
	info_crt_instance.pNext = (void*) &info_debug_instance;
#endif
	
	VkResult r = vkCreateInstance(&info_crt_instance, nullptr, &vk_instance);
	VERIFY(r, "Failed to create Vulkan instance.")
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Initialized Vulkan application and instance." << std::endl;
#endif	

#ifndef NDEBUG
	if(!setup_validation_layers()) return false;
#endif
	if(!create_window_surface(window)) return false;
	if(!init_vulkan_device()) return false;
	
	delete[] extensions;
	delete[] layers;
	
	return true;
}

VkPhysicalDevice get_selected_physical_device()
{
	return vk_physical_device;
}

VkSurfaceKHR get_window_surface()
{
	return vk_surface;
}

VkDevice get_device()
{
	return vk_device;
}