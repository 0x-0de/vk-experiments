# 01 - Hello Triangle
Seeing as how this is the first Vulkan experiment, these notes will by far be the most extensive of all the learning exercises contained within this repository, as this program is being developed alongside my first (well, more like third) visit to the [Khronos vulkan tutorial](https://docs.vulkan.org/tutorial/latest/02_Development_environment.html). I'm very much still in the learning process myself, and I'm hoping to teach myself the process necessary to render a simple triangle.

Most of these notes will be a collection of shortened, annotated, abbreviated versions of what's on that tutorial, which is licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/). As such, all contents of these notes are provided under the same license. These notes are intended for personal use/documentation only. *Please follow the official Vulkan tutorial itself if you are interested in learning the API for yourself.*

For these experiments, the only external library outside of the Vulkan SDK that I plan on using is GLFW, to get a simple window on which to put the Vulkan rendering framebuffer. I've developed my own linear algebra library, so using GLM will not be necessary. GLFW is distributed under the zlib license. More information can be found in the root directory's ```licenses``` folder.

## Introduction and Setup
[Vulkan](https://www.vulkan.org/) is a graphics API created by the [Khronos Group](https://www.khronos.org/), who had previously created and maintained [OpenGL](https://www.opengl.org/). This new API, which saw its initial release in February 2016, provides a better and more detailed abstraction of modern graphics cards, allowing developers to more easily control driver behavior and card performance on different systems rather than having to worry about how different GPUs will behave. Vulkan, unlike Direct3D and Metal, are cross platform, supporting Windows, Linux, and Android. The big tradeoff for this remarkable improvement in developer freedom and control is that Vulkan's API is significantly more verbose than OpenGL, and far more work needs to be done to define the precise behavior of your graphics application, rather than the relative hand-holdiness of OpenGL. OpenGL itself will be maintained for a long time, so moving to Vulkan is by no means a requirement for the foreseeable future.

Here is a link to the download page for the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows).

The first thing to do after installing the Vulkan SDK is to run the ```bin/vkcube.exe``` program. The user should see a spinning cube with the LunarG logo plastered on each visible side. The purpose of this is to verify that the developer's graphics drivers support at least Vulkan 1.0.

[GLFW](https://www.glfw.org/) is a simple library for "creating windows, contexts, and surfaces, and recieving inputs and events," according to their main page. GLFW is intended to be used for OpenGL, as part of the Vulkan process is creating the surface and context for which Vulkan will output rendered images. However, GLFW does not mandate the creation of an OpenGL context, and so we'll see how to attach the Vulkan process to it soon enough.

To compile a Vulkan program, you'll need a 64-bit compiler that uses the C++17 standard (or later). I'm using MinGW-w64, which uses GCC 13.1. The project requires the linking of the vulkan-1.dll (located in System32 on Windows) and the glfw3.dll found in a compiled GLFW binary download (unless you want to do it yourself), and both the Vulkan SDK and GLFW also have include headers. Check the reference makefile for specifics on how this project is compiled.

Here's some starter code to make sure everything is set up and working properly:

```
#include "../include/vulkan/vulkan.h"
#include "../include/GLFW/glfw3.h"

#include <iostream>

int main()
{
    glfwInit();
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Test window", nullptr, nullptr);
    
    uint32_t extensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensions, nullptr);

    std::cout << "Vulkan extensions supported: " << extensions << "." << std::endl;

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
```
Once run, the program showed a window drawing a black screen, with the following print statement:
```
Vulkan extensions supported: 18.
```
Any value greater than 0 (but not indicative of an unsigned integer underflow) is considered non-faulty.

If this code runs properly, the development environment has been set up properly. For more details on setting up the development environment, please refer to the [official Khronos tutorial on how to do so](https://docs.vulkan.org/tutorial/latest/02_Development_environment.html).
## Journey to the Triangle
The first version of OpenGL released in 1992, seven years before Nvidia would debut the [GeForce 256](https://en.wikipedia.org/wiki/GeForce_256), widely considered to be the first GPU. Back in the 1980s, computer graphics hardware was highly specialized, and software developers would often have to write custom interfaces, APIs, and drivers for each piece of hardware capable of rendering graphics, as it was for most other things. In the late 80s and early 90s, as commercial hardware became more and more standardized, and demand for a more general-purpose rendering architecture increased, [SiliconGraphics](https://en.wikipedia.org/wiki/Silicon_Graphics) introduced IRIS GL, which supported an immediate mode form of rendering for the IRIX operating system, which initially released in 1988. Pressure from competitors such as IBM and Sun Microsystems led them to open source a version of IRIS GL in 1992, becoming the first release of OpenGL.

OpenGL v1.1 would release five years later in 1997, including support for texture objects and vertex arrays. More updates to the standard in the following years would introduce 3D textures, multitexturing, vertex buffer objects, among other things. OpenGL 2.0 would release in 2004, introducing GLSL and allowing developers to program their own vertex and fragment shaders. In July 2006, around the same time as the release of OpenGL 2.2, ownership of the OpenGL API standard was transferred to the Khronos Group. OpenGL 3.3, released in 2010, was considered the "standard" modern version that ought to be supported by most systems prior to the release of Vulkan. As of 2024, the latest release of OpenGL is 4.6, released in July 2017.

As the development of OpenGL continued throughout the 2000s and 2010s, graphics cards were beginning to become more and more generalized and versatile. Integrating this new versatility with the existing API would facilitate a lot of guesswork on the part of the API developers, leading to inconsitent behavior for OpenGL programs on different hardware. One common example of this is the accepted syntax used for shaders (**anecdotal example**: My laptop has an integrated Intel GPU and a discrete Nvidia GPU. The Intel GPU seems to accept that ```vec3.length``` is a function, although it's return value appears to exhibit undefined behavior. My discrete GPU, on the other hand, mandates that ```vec3.length``` is not a function, and that ```length(vec3)``` must be used instead).

On top of all of this, one other major feature of graphics hardware in the 21st century is the proliferation of mobile GPUs on smartphones and other mobile devices, whose architectures differ greatly from conventional PC GPUs.

Vulkan attempts to address these problems by trying to 'meet the hardware where it's at', rather than fit the hardware to work with the API like with OpenGL. This approach requires much more configuration and development on the part of the graphics developer, however it has the purpose and benefit of acknowledging that modern graphics cards are far more general-purpose than before, allowing for the unification of render and compute functionality, the submission of GPU commands to multiple queues in parallel, and switching to an ahead-of-time approach to shader compilation using a standardized bytecode format, the compiler for which should come with the SDK.

In an ideal, well-behaved Vulkan program, the following is a big-picture look at the steps required to draw a triangle onto the screen:

### Step 1: Instance creation, validation layers.
A Vulkan application must create an instance of the API through a ```VkInstance```, which describes the application you'll be creating, as well as any Vulkan extensions you'll be using. 

This step may also involve the optional process of setting up custom validation layers, or using the ones provided with the Vulkan SDK. These validation layers are like advanced error checkers for Vulkan applications, able to detect, among many other things, if a Vulkan object hasn't been properly deallocated, and it's recommended that they be used for debug builds of Vulkan programs.

### Step 2: Physical device selection and logical device creation.
Now that the instance is created, the next step is to select a ```VkPhysicalDevice```, representing a piece of physical hardware (typically a GPU) that supports Vulkan operations. These device objects can be queried for desirable properties such as VRAM size, available queue families (see step 2), and other device capabilities.

Once the right hardware is selected, a ```VkDevice``` is created to describe the ```VkPhysicalDeviceFeature```s that will be in use, such as 64-bit floating point precision or multi-viewport rendering.

Vulkan uses queues to execute commands asynchronously. These queues belong to queue families, which essentially define the capabilities of each queue, whether it's used for graphics, compute, memory transfer, or other operations, or all or none of the above. Queue families belong to the hardware itself, and can be queried in the previous step to make sure the chosen device supports the queue capabilities that the developer wants.

### Step 3: Window surface targetting.
This is where GLFW comes in. Unless you're interested in offscreen rendering, the Vulkan program will need a target or destination for rendered frames to be displayed onto. To tether the Vulkan program to the output window, a ```VkSurfaceKHR``` object is required. The ```KHR``` suffix indicates that the feature is part of a Vulkan extension. Since Vulkan is platform-agnostic, the standardized WSI (Window System Interface) extension is needed to interact with the OS's window handler.

### Step 4: Setting up the swap chain.
After the window surface has been properly targetted, the program needs a swap chain to use. A swap chain, defined using ```VkSwapchainKHR```, is a set of targets to render to, and it's purpose is to make sure that the current image being displayed on-screen isn't also being currently rendered to.

The way it works is this: the swap chain needs to be requested for an image to render to. Once the image has been provided and rendered to, we then return it to the swap chain to be rendered at some point. Swap chains provide a number of render targets, and define a set of conditions for presenting finished images to the screen.

### Step 5: Image views, framebuffers, and render passes.
Render passes contain a context for how images can be used, will determine how images (or image views, in this case) will be used and treated in render operations. In the case of the triangle, we just want to use a single render pass, using the GPU's color target, and we also want the image to be cleared to a single, solid color before the drawing operation begins.

Image views reference a specific part of the image to be used in a render pass, acting as a wrapper for the images in the swap chain. A framebuffer is yet another wrapper on top of that, but the framebuffer is what binds an image view to a specific render pass.

### Step 6: The graphics pipeline.
The graphics pipeline in Vulkan is described with a ```VkPipeline``` object, containing numerous details of the graphics card such as its viewport size, depth buffer operation, and ```VkShaderModule``` objects created from precompiled shaders. Nearly all of these data points must set in advance, with only a few basic configurations, like viewport size and clear color, able to be changed dynamically. This means that if you intend on using multiple shader programs, you'll also need to use multiple pipelines. The tradeoff being that each pipeline facilitates far more optimization opportunities than what would be previously provided under the OpenGL approach.

For this example, the vertex coordinates of the triangle will be embedded in the vertex shader. 

### Step 7: Command pools and command buffers.
Operations to be submitted to a queue need to first be recorded into a ```VkCommandBuffer```, which is allocated from a ```VkCommandPool```, which is associated with a specific queue family.

To draw a simple triangle, we just need to record a simple command buffer, with the following operations:

1. Begin render pass.
2. Bind the graphics pipeline.
3. Draw 3 vertices.
4. End render pass.

We can simply record a command buffer for every possible image to call upon at draw time.

### Step 8: The main loop.
With the graphics pipeline, drawing commands, and swap chain presentation setup complete, we can create the main loop, which runs as follows:

1. Get the next available image from the swap chain, via ```vkAcquireNextImageKHR```.
2. Select the appropriate command buffer for that image, and execute it with ```vkQueueSubmit```.
3. Return the image to the swap chain for presentation with ```vkQueuePresentKHR```.

There's a little bit more to it than that, as since queue commands are executed asynchronously, we must make use of synchronization objects such as semaphores to ensure correct order of execution. Steps 2 and 3 need to wait for their previous step to finish before they can execute.

## API structure
Most functions, especially creation or initialization functions in Vulkan, require the use of structs to pass information into them. These structs contain information about certain objects or things in the API, and essentially replace the many parameters you could find in OpenGL functions (such as ```glTexImage2D```).

These structs will typically have an ```sType``` member, which will specify the type of structure being passed in, as well as a ```pNext``` member which points to an extension structure, but will usually be ```nullptr```. Functions that create or destroy an object can also have a custom allocator for driver memory (under a ```VkAllocationCallbacks``` parameter), but for the sake of this demo will also always be ```nullptr```. These structs can have default values, but they must be initialized with an empty constructor (two curly brackets containing nothing), like so:
```
VkApplicationInfo info{};
```
As has likely been implied, every Vulkan object created needs to be explicitly destroyed when it is no longer needed. Creation and allocation function identifiers typically begin with ```vkCreate``` or ```vkAllocate```, while the required destruction functions begin with ```vkDestroy``` and ```vkFree```.

Most functions in Vulkan return a VkResult, an enum that can either be ```VK_SUCCESS``` or some error code. The [Vulkan Specification](https://docs.vulkan.org/guide/latest/vulkan_spec.html) has more detail on what these error codes mean for each function that can return them.

As stated in step 1 earlier, while Vulkan's high performance and low driver overhead mandates a very limited error checking system, the API allows you to optionally enable validation layers, which can run extra checks on function parameters and track memory management problems. They can be enabled during development and disabled for release with no altered behavior to the program itself. The Vulkan SDK provides a standard set of validation layers that can be used for debug builds. To use validation layers, a callback function must be registered to receive debug messages from the layers.

## Initializing GLFW
GLFW, in comparison to Vulkan, is a very simple library, and getting a single window to show up upon running the program is quite easy.

Firstly, the required header is ```glfw3.h```, which will need to be included after ```vulkan.h``` from the Vulkan SDK.
```
#include "../include/vulkan/vulkan.h"
#include "../include/GLFW/glfw3.h"
```
GLFW contains a macro which can automatically include that vulkan header with an include of ```glfw3.h```, assuming that the "vulkan" folder is located in the root of your include directory, and that your compiler or build system checks for included files in said directory.
```
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```
I might choose to include the libraries in this manner if I could only figure out how to wrangle GCC make (the build system I'm using) to accept the "include" folder as a search directory. But, honestly, I think the top includes feel more organized and explicit.

The very first call to GLFW needs to be ```glfwInit()```, which initializes GLFW. Then, we need to disable the automatic creation of an OpenGL context with ```glfwWindowHint```. ```glfwWindowHint``` is a function which sets certain parameters or attributes of a GLFW window prior to being created. These functions must go before the creation of the ```GLFWwindow```.
```
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
```
Since window resize handling requires special care (see step 6), I've opted to disable it for the time being.

Now that those two function calls are out of the way, we can create the actual window.
```
GLFWwindow* window = glfwCreateWindow(1280, 720, "Test window", NULL, NULL);
```
The first three parameters of the function specify the window's initial width, height, and title. The fourth and fifth are optional - the fourth parameter lets you specify a specific monitor to show up on, and the fifth lets you share data with a different window, which is only relevant when working with OpenGL.

When the program ends, you'll need to clean up by calling ```glfwDestroyWindow``` and ```glfwTerminate```.
```
glfwDestroyWindow(window);
glfwTerminate();
```
If you run the program at this point, you might see a blip of a window, but you won't see it for long, since the program ends immediately after it creates a window. To keep the application running until the window is closed by the user (or until some error occurs), you can add an event loop after the window was created like this:
```
while(!glfwWindowShouldClose(window))
{
    glfwPollEvents();
}
```
The ```glfwWindowShouldClose``` function should be self-explanatory. ```glfwPollEvents``` checks for user input events, such as pressing certain keys on the keyboard or buttons on the mouse. This demo won't use any of these events, but this function is still required for a proper loop with GLFW.

Running the program at this point, you'll notice the screen presents a blank Windows window rather than a black screen like with the test code, and that's because the black screen was the blank OpenGL context on display.

## The Instance and GLFW extensions
The Vulkan instance is responsible for "creating" the Vulkan application. It initializes the Vulkan library with parameters specific to your program, and serves as the connection between you and Vulkan.
```
VkInstance instance;
```
Creating the application requires you to specify some details about it to the driver using the ```VkApplicationInfo``` struct.

This data is optional, but might provide some useful information to the GPU drivers about the application.
```
VkApplicationInfo info_app{};
info_app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
info_app.pApplicationName = "Hello Triangle";
info_app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
info_app.pEngineName = "No Engine";
info_app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
info_app.apiVersion = VK_API_VERSION_1_0;
```
This is just an example. The ```VK_MAKE_VERSION``` macro returns a version code created with those values. The only real consequential value here is ```apiVersion```, which tells the GPU drivers which version of Vulkan we want to use. In my case, I want to stick with 1.0.

On the other hand, this next struct is not optional. The ```VkInstanceCreateInfo``` struct contains some direct info about the application, including which extensions we're going to use. Since we are going to use some extensions so that Vulkan can render to the GLFW window.
```
VkInstanceCreateInfo info_inst_create{};
info_inst_create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
info_inst_create.pApplicationInfo = &info_app;
info_inst_create.enabledLayerCount = 0;
```
Those first two members should be pretty self-explanatory. ```enabledLayerCount``` refers to the number of global validation layers to enable. For now, this value is being set to 0.

To get the required extensions for GLFW, GLFW provides a nice helper function to get the required extensions for the current context (as in, whatever platform you're running on). 
```
uint32_t glfw_req_extensions = 0;
const char** glfw_extensions;

glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_req_extensions);

info_inst_create.enabledExtensionCount = glfw_req_extensions;
info_inst_create.ppEnabledExtensionNames = glfw_extensions;
```
Extension identifiers are strings, which is why their names are used here. With that done, we can now create the instance.
```
VkResult result = vkCreateInstance(&info_inst_create, nullptr, &instance);
```
This pattern of passing the struct, followed by the allocation callback, followed by a pointer to the object itself is done by a lot of creation functions in Vulkan. As stated before, all Vulkan objects created in this way must eventually be destroyed with the appropriate destruction function:
```
vkDestroyInstance(instance, nullptr);
```
And like with creation, many of these deletion functions will follow this pattern as well.

Since ```vkCreateInstance```, like most other functions in Vulkan, returns a ```VkResult```, it would be good to verify the result returned ```VK_SUCCESS```.
```
if(result != VK_SUCCESS)
{
    std::cerr << "[ERR] Failed to create Vulkan instance." << std::endl;
    std::cerr << "Error code: " << result << std::endl;

    /* Error response - return an error value or throw a runtime error. */
}
```
If you want to know what extensions in Vulkan are available, you can query the extensions like this:
```
uint32_t extensions_total = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extensions_total, nullptr);

VkExtensionProperties* extension_properties = new VkExtensionProperties[extensions_total];
vkEnumerateInstanceExtensionProperties(nullptr, &extensions_total, extension_properties);
```
The ```VkExtensionProperties``` struct contains the name and version of it's reference extension. You could check if the extensions required by GLFW are contained within this list, and then throw an error if they're not present.

## Validation Layers
Validation layers are optional components that apply additional error-checking or validation operations to Vulkan function calls. They can be thought of as an additional route that the function call takes before the real function is called. They can be enabled by the developer for debug builds of a project, and disabled for release builds to allow for minimal CPU overhead.

The Vulkan SDK provides it's own standard set of validation layers, which check for issues such as memory leakage, thread safety, and verifying struct members and function parameters with the Vulkan specification to detect misused data. One important aspect of the Vulkan SDK validation layers is that they're only available on PCs with the Vulkan SDK installed.

The name of the standard validation layer which the LunarG Vulkan SDK provides is ```VK_LAYER_KHRONOS_validation```. You can check and see if it's supported with ```vkEnumerateInstanceLayerProperties```, in a similar vein to checking for Vulkan extensions with ```vkEnumerateInstanceExtensionProperties```.
```
uint32_t layer_count = 0;
vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

VkLayerProperties* layers = new VkLayerProperties[layer_count];
vkEnumerateInstanceLayerProperties(&layer_count, layers);
```
The ```VkLayerProperties``` struct, like the ```VkExtensionProperties``` struct, contains information about each layer, such as the name and version. From there, it should be pretty simple to loop through each and check if you have ```VK_LAYER_KHRONOS_validation``` or not.

Once you've verified that you have it, you can enable it while creating the instance by updating the ```enabledLayerCount``` attribute of the ```VkInstanceCreateInfo``` struct, and setting the ```ppEnabledLayerNames``` to a const char* array containing the names of each layer you'd like to apply.
```
if(validate)
{
    info_inst_create.enabledExtensionCount = VALIDATION_LAYER_COUNT;
    info_inst_create.ppEnabledLayerNames = validation_layers;
}
else
{
    info_inst_create.enabledLayerCount = 0;
}
```
We're not done yet. Remember that these validation layers need some sort of callback, the equivalent of a 'catch' statement to the layers' 'try'. Not all messages will necessarily indicate fatal errors, so it's important to catch only the messages that are important to you.

Vulkan provides another useful extension for this, which is the ```VK_EXT_debug_utils``` extension. This extension allows developers to easily set up a debug messenger which prints to the standard output. Vulkan wrapped this name into a macro named ```VK_EXT_DEBUG_UTILS_EXTENSION_NAME```. Remember to add that extension to the required GLFW extensions, as well as any others you might be using, if you're using the validation layers.

Now we can create that callback function.
```
static VKAPI_ATTR VkBool32 VKAPI_CALL vl_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    std::cerr << "[VULKAN VALIDATION LAYER]\n" << callback_data->pMessage << std::endl;
    return VK_FALSE;
}
```
This function has the ```PFN_vkDebugUtilsMessengerCallbackEXT``` prototype. The ```VKAPI_ATTR``` and ```VKAPI_CALL``` macros ensure that Vulkan can call the function.

The ```message_severity``` parameter determines the importance or severity of the message, and can have one of four values:

- ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT``` - Diagnostic, comes after a warning or error.
- ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT``` - Information, like the creation of a resource.
- ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT``` - Warning about potential causes of bugs or errors in a program.
- ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT``` - Warning about behavior that is invalid and likely to cause a crash.

These values are set up so that each level is greater than the previous one numerically, allowing you to do simple comparison operations with them.
```
if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
{
    //Display message.
}
```
The ```message_type``` parameter can have one of three values:

- ```VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT``` - This message indicates a violation or possible mistake in the program, drawn from a comparison to the specification.
- ```VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT``` - This message indicates a potential sub-optimal use of Vulkan.
- ```VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT``` - Some other event has happened, worthy of being printed but unrelated to the first two message types.

The ```callback_data``` parameter contains a struct containing the details of the message, which includes the message itself (```pMessage```), an array of Vulkan objects related to the message (```pObjects```), as well as the number of objects in said array (```objectCount```).

The ```user_data``` parameter contains a pointer to some data, specified during the setup of the callback, allowing you to pass your own data to the function.

In case you're wondering, the function returns a value indicating whether the Vulkan call that triggered the layer callback should be aborted (if ```VK_TRUE``` then yes). This is generally only used to test the validation layers themselves, so in 99% of cases the function should always return ```VK_FALSE```.

At this point, we need to tell Vulkan about the callback function, using a handle that needs to be explicitly created and destroyed similar to a ```VkInstance```.
```
VkDebugUtilsMessengerEXT vl_debug_messenger;
```
Then, a similar creation info struct called ```VkDebugUtilsMessengerCreateInfoEXT``` needs to be created and filled out.
```
VkDebugUtilsMessengerCreateInfoEXT info{};
info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
info.pfnUserCallback = vl_debug_callback;
```
The ```messageSeverity``` and ```messageType``` attributes specify which types and severities of messages should even bother triggering the callback at all. In most cases, you'll likely want to ignore the ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT``` severity, as it trips every time a new Vulkan resource is created. For the sake of testing, I've included it here. The ```pfnUserCallback``` attribute should be self-explanatory - this is where the pointer to your callback function should go.

Because the creation function, ```vkCreateDebugUtilsMessengerEXT```, is an extension function, we actually need to get its address since it isn't automatically loaded.
```
auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
```
```vkGetInstanceProcAddr``` returns a pointer to the procedure, or function, with the name specified with the second parameter, or ```nullptr``` if it can't load or locate it. From here, you can call ```func``` as you would the regular function.
```
VkResult result = func(instance, &info, nullptr, &vl_debug_messenger);
```
Like before, this resource will need to be destroyed eventually. Just before the instance is destroyed, use this same method to access and call ```vkDestroyDebugUtilsMessengerEXT```.
```
auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
if(validate && func != nullptr) func(instance, vl_debug_messenger, nullptr);
```
While this process is now technically complete, there's just one small problem. In case you haven't noticed, the successful creation of a Vulkan instance is required for the validation layers to be applied. The problem with this is that we can't validate or debug issues with instance creation or deletion, since validation layer handles need to be deleted before the instance is deleted.

Fortunately, there is a way to create a seperate messenger specifically for those function calls. All you have to do is pass a ```VkDebugUtilsMessengerCreateInfoEXT``` struct into the ```pNext``` field of the ```VkInstanceCreateInfo``` you're using. This struct should have the same parameters that you set before, although if you want to change the allowed severity and types of messages then go for it.

Running the program at this point, you should see a lot of boiler-plate messages about layer libraries being loaded. Here are a few that I got:
```
[VULKAN VALIDATION LAYER]
Checking for Layer Manifest files in Registry at HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers
[VULKAN VALIDATION LAYER]
windows_add_json_entry: Located json file "C:\Windows\System32\DriverStore\FileRepository\nvdm.inf_amd64_ff8b71fcd7c27050\nv-vk64.json" from PnP registry: E
[VULKAN VALIDATION LAYER]
Located json file "C:\Program Files\RenderDoc\renderdoc.json" from registry "HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers"
[VULKAN VALIDATION LAYER]
Located json file "C:\Program Files (x86)\Overwolf\0.260.0.8\ow-vulkan-overlay64.json" from registry "HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers"
```
There's a lot of boiler-plate messages that print with the ```VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT```, which is why I'm going to disable it from here on out. I also choose to ignore messages with a general type and verbose severity.

**NOTE:** It seems like either Vulkan now implicitly destroys all resources when the instance is deleted, or when the program terminates, or the validation layer is failing to detect leaked resources. Will have to investigate this further.

## Selecting and Preparing the Physical Device
After we have the instance, we need to look for a graphics card or some other hardware capable of supporting the Vulkan features that are necessary. We can use multiple graphics cards, but in most cases only 1 is required.

Information about this physical device that we'll select will be stored in a ```VkPhysicalDevice``` handle.

```
VkPhysicalDevice physical_device;
```
Like with instance extensions and layers, we can query the number of available physical devices to choose from.
```
uint32_t device_count = 0;
vkEnumeratePhysicalDevices(instance, &device_count, 0);

if(device_count == 0)
{
    std::cerr << "[ERR] Unable to locate any GPUs that support Vulkan." << std::endl;
    return false;
}

VkPhysicalDevice* physical_devices = new VkPhysicalDevice[device_count];
vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);
```
From here it's a matter of looking at each device, getting the properties, features, and queue families of each device, and then choosing which is best. The approach I use is to give each GPU a score based on how well it would run the program, and then choose the one with the highest score.

To get the device properties and features, you can use a ```VkPhysicalDeviceProperties``` and ```VkPhysicalDeviceFeatures``` struct respectively.
```
VkPhysicalDeviceProperties properties;
VkPhysicalDeviceFeatures features;

vkGetPhysicalDeviceProperties(device, &properties);
vkGetPhysicalDeviceFeatures(device, &features);
```
The ```VkPhysicalDeviceProperties``` struct contains basic information about the hardware, usually directly relating to its manufacturer. Of note are the ```deviceName``` and ```deviceType``` fields, the former containing a human-readable string identifying the model of the graphics card and the latter containing it's type, whether it's discrete, integrated, or virtual.

The ```VkPhysicalDeviceFeatures``` struct contains more pertinent information for graphics developers, such as whether the GPU supports depth bias clamping, geometry shaders, or 64-bit floating point precision.

We're also at the stage where we can begin to look at queue families. Once again, these can be enumerated as follows:
```
uint32_t queue_family_count = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[queue_family_count];
vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
```
You can check what commands each queue family supports by looking at the ```queueFlags``` attribute. If the ```VK_QUEUE_GRAPHICS_BIT``` is enabled on any of them, then that queue family supports graphics commands.
```
if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
{
    //Supports graphics!
}
```
We'll need to pay attention to which queue families support which commands much later on when we get to creating command pools and buffers, so it's probably worth storing a bit of information about these queue families, namely, which queue families support which commands. For now, though, it's good enough to know at least one queue family supports graphics commands.

Here's the simple code I use to scan and score each physical device:
```
typedef struct
{
    //These values are optionals to indicate if the GPU even has these queue families.
    std::optional<uint32_t> graphics_index;
    std::optional<uint32_t> transfer_index;
} queue_family_indices;

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
        else if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            indices.transfer_index = i;
        }
    }

    delete[] queue_families;
    return indices;
}

[...]

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
    if(!indices.graphics_index.has_value()) score = 0; //Unusable - has no graphics capabilities. 

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
```
I chose to use the tutorial's method of taking a struct with optionals to store the relevant queue family information. This has the added benefit of each GPU having a 'null' value for each queue family index, indicating whether the GPU supports those commands at all. You'll notice I'm also storing an index for transfer commands, this will be necessary later.

Now we can infer which physical device is the most suitable for this program involves the creation of a logical device handle, which will tell Vulkan both what GPU we're using as well as what features from the GPU we intend to use.
```
VkDevice device;
```
I guess it really was a good idea to store that queue family information, because this is the first place we'll be using it, as creating the ```VkDevice``` handle also creates queues that you'll need to interact with, so we'll need to tell it what kinds of queues we want, and from which queue families to allocate said queues from.

To start, we need one or more ```VkDeviceQueueCreateInfo``` structs to tell Vulkan and the ```VkDevice``` which queues we'd like to create. If you're using multiple, you'll need to bundle them into an array.
```
queue_family_indices selected_indices = find_physical_device_queue_families(physical_device);
float queue_priority = 1.f;

VkDeviceQueueCreateInfo info_queue{};
info_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
info_queue.queueFamilyIndex = selected_indices.graphics_index.value();
info_queue.queueCount = 1;
info_queue.pQueuePriorities = &queue_priority;
```
Note that this struct is made in reference to a queue family, meaning we can request multiple queues from said family, using the ```queueCount``` attribute. Each of these queues can also be assigned a priority, as specified by ```pQueuePriorities```. This is a required field to define, even if you're just using one queue.

Specifying which features of the physical device is done with a ```VkPhysicalDeviceFeatures``` struct. For now, we'll default initialize it, leaving everything set to ```VK_FALSE```.
```
VkPhysicalDeviceFeatures info_features{};
```
Now, we can fill out the ```VkDeviceCreateInfo``` struct.
```
VkDeviceCreateInfo info_device{};
info_device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
info_device.queueCreateInfoCount = 1;
info_device.pQueueCreateInfos = &info_queue;
info_device.pEnabledFeatures = &info_features;
info_device.enabledExtensionCount = 0;
```
Logical devices have their own extensions, one of which we'll eventually be using is ```VK_KHR_swapchain``` for presenting rendered images to the GLFW window. For now, I'll leave it at 0.

Older specifications of Vulkan also had device-specific validation layers. This is no longer the case, and now the instance validation layers are the only ones to be referenced. In up-to-date drivers, the following code will do nothing, but it may still be worth implementing for older hardware.
```
if(validate)
{
    info_device.enabledLayerCount = 1;
    info_device.ppEnabledLayerNames = validation_layers;
}
else
{
    info_device.enabledLayerCount = 0;
}
```
At last, that's all the information we need to create the logical device handle.
```
VkResult result = vkCreateDevice(physical_device, &info_device, nullptr, &device);
```
Don't forget to destroy the handle when the program terminates.
```
vkDestroyDevice(device, nullptr);
```
This essentially concludes the basic setup for Vulkan. We have now created a Vulkan instance, specified which GPU to use, and what features of the GPU we would like to work with, thus completing steps 1 and 2 of the triangle journey. In OpenGL, all of this would be handled by either the internal library or the operating system, or both.

The remainder of the work here is going to be in setting up the Vulkan renderer, beginning with creating a swap chain and tying it to the GLFW window, and then creating the actual graphics pipeline, before finally implementing an actual draw loop with command buffers.

## Setting up the Window Surface
This is going to be a nice, short step that really could've been done as part of the earlier setup portion of the process, but doing it here, as noted by the tutorial, is convenient as most of the concepts required to understand the window surface have been covered already.

Vulkan is platform agnostic, so it cannot interface with the window system of the current OS directly. You need some of Vulkan's WSI (Window System Interface) extensions to do that. Fortunately, we already enabled those extensions in the first step, using ```glfwGetRequiredInstanceExtensions```. The only extension required here for now is ```VK_KHR_surface```, which provides the most basic of window surface functionality.

This step could've been done earlier, as window surface creation should happen before physical device selection, since it can directly influence it as we'll see.
```
VkSurfaceKHR surface;
```
While the object itself is platform agnostic, the process of creating said object isn't. Fortunately, GLFW handles most of those platform-specific details for us. If you want an example of how you might implement surface creation on Windows devices without GLFW, check the tutorial. It's not a super involved process, at least for Windows.

GLFW provides a simple function which returns a ```VkResult```, that acts like the standard kind of Vulkan creation function that we've seen so far, only we don't need to pass in a struct, instead just passing in the ```GLFWwindow*``` we're using.
```
VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
```
GLFW doesn't provide any special functions for deleting the window surface, because it doesn't need to. The Vulkan destroy function works fine here.
```
vkDestroySurfaceKHR(instance, surface, nullptr);
```
Remember, not only do all created or allocated Vulkan resources need to be deleted, they also need to be deleted in the proper order. Here, I'm deleting the surface after deleting the logical device handle.

And that's it. We made the surface handle. But there's a bit more work to do now. Like I said, this can and will influence physical device selection. And that's because not only are we looking for a GPU which can handle graphics commands, we're also looking for one that can actually present these graphics to a window surface. *That's why we want to do this before selecting a physical device and creating the logical device handle.*

Presentation is a type of command, which can be supported by a queue family. Checking for if a specific queue family can support your window surface is a bit of a different process, but fortunately, Vulkan provides an easy function to use for this.
```
/* I added a std::optional<uint32_t> for the presentation index to the queue_family_indices struct. */

VkBool32 supports_presentation;
vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_presentation);

if(supports_presentation) indices.presentation_index = i;
```
While it's likely (and also optimal) if a queue family can support both graphics and presentation at the same time, it's also possible that this will result in the logical device needing queues from multiple different families.

The approach I use is very similar to the approach adopted by the tutorial - use a ```std::set``` to take in the values of the queue family indices that I have stored to ensure I'm not wasting a queue, create a ```VkDeviceQueueCreateInfo``` array the size of the set, populate each, and tie them with the ```VkDeviceCreateInfo```.
```
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

[...]

info_device.queueCreateInfoCount = queue_indices.size();
info_device.pQueueCreateInfos = info_queue;
```
Now the logical device and the window surface have been fully created and tethered together. On to step 4.
## The Swap Chain
The first "big" step. Creating the swap chain is a bit of an involved process in Vulkan, beginning with checking to see if Vulkan even supports it, to seeing exactly what swap chain capabilities and formats it supports, and choosing said capabilities and formats before creating the swap chain itself and accessing the images within.

The swap chain extension is device-specific, and some graphics cards won't support it for one reason or another, so it exists as a device extension rather than a Vulkan extension. Therefore, we have to add yet another step to process of physical device selection and logical device creation - querying the extensions of each device and checking which ones support ```VK_KHR_swapchain```.

We've done a similar process before, so I'll skip the details and just show the function.
```
#define DEVICE_EXTENSION_COUNT 1

const char* device_extensions[DEVICE_EXTENSION_COUNT] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME //"VK_KHR_swapchain"
};

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
```
I added this to the physical device scoring process, resetting the score and rendering the device unusable if it doesn't support these extensions.
```
if(!indices.graphics_index.has_value() || !check_device_extension_support(physical_devices[i])) score = 0; //Unusable.
```
And then enabled the device extension in a similar fashion to enabling instance extensions.
```
info_device.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
info_device.ppEnabledExtensionNames = device_extensions;
```
Simple enough. Here's where the fun part begins. Now that we know the chosen device will support a swap chain, we need to know the details. In particular, we're interesting in three things:

- A ```VkSurfaceCapabilitiesKHR``` struct containing broad details about the capabilities that the swap chain can support, such as the maximum number of images, or the min/max resolution of each image.
- One or more supported surface formats, stored as ```VkSurfaceFormatKHR``` structs. These surface formats are a collection of pixel formats and color spaces.
- One or more available presentation modes, stored as a ```VkPresentModeKHR``` enum. Presentation modes determine the conditions necessary for an image to be presented to the screen, or be swapped with a different image.

This is another fairly straightforward process. Similarly to getting queue family information, I'm creating a function which will return a struct containing information about what the swap chain supports.
```
typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentation_modes;
} swap_chain_support;
```
Then, we can use ```vkGetPhysicalDeviceSurfaceCapabilitiesKHR```, ```vkGetPhysicalDeviceSurfaceFormatsKHR```, and ```vkGetPhysicalDeviceSurfacePresentModesKHR``` to query the capabilities, formats, and presentation modes a swap chain from the selected physical device would support. This is similar to querying for extensions in the case of the latter two.
```
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
```
It might be a good idea to have a look at these supported swap chain capabilities when selecting a physical device. In my case, I just want to check that there's at least one supported format and one supported presentation mode.
```
swap_chain_support scs = query_swap_chain_support(physical_devices[i]);
if(scs.formats.size() == 0 || scs.presentation_modes.size() == 0) score = 0;
```
Now we can make a decision on which of these formats and presentation modes we'll want to use. For formats, each ```VkSurfaceFormatKHR``` struct contains a ```format``` and ```colorSpace``` attribute which contains information about the pixel format and color space respectively. In following with the guide of the tutorial, I'm choosing to use the standard sRGB color space format if it's available, otherwise just whatever's up front.
```
VkSurfaceFormatKHR choose_swap_chain_format(swap_chain_support supported)
{
    for(auto format : supported.formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return supported.formats[0];
}
```
Presentation modes require a bit more explanation. As of Vulkan 1.0, there are four available presentation modes:

- ```VK_PRESENT_MODE_IMMEDIATE_KHR``` - Any submitted images to the presentation queue will be displayed immediately. This will likely result in some tearing.
- ```VK_PRESENT_MODE_FIFO_KHR``` - The swap chain is queue. When the display is refreshed, the program takes the image at the front and disposes it, moving the next available image up front. When the program submits rendered images, they end up at the back of the queue. If the queue is full, the program must wait, causing some latency. This mode is most similar to V-sync.
- ```VK_PRESENT_MODE_FIFO_RELAXED_KHR``` - Same as FIFO, but if the application is late, and the queue is empty at the next display refresh, the image is transferred right away when submitted, which may lead to tearing.
- ```VK_PRESENT_MODE_MAILBOX_KHR``` - Similar to FIFO but if the queue is full when an image is submitted, images that are already queued are replaced with newer ones. Commonly referred to as "triple buffering."

If a graphics card supports a Vulkan-extended swap chain, then it is guarunteed to support ```VK_PRESENT_MODE_FIFO_KHR```.

It's important to bear in mind the capabilities of your system when choosing a presentation mode especially, since buffering images will be the source of most of your energy consumption at runtime. Using fewer images in the swap chain, and letting the program "relax" if the queue is full would be a good decision for mobile devices where energy usage is a real concern (```VK_PRESENT_MODE_FIFO_KHR```). If you want minimal latency while avoiding tearing, you may want to instead go with ```VK_PRESENT_MODE_MAILBOX_KHR``` if your graphics card supports it.
```
VkPresentModeKHR choose_swap_chain_present_mode(swap_chain_support supported)
{
    for(auto present_mode : supported.presentation_modes)
    {
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
```
The last thing we need to decide are some of the capabilities we want to use, namely, the resolution, or extent, of the images themselves.

Surprisingly, this can be a bit of an involved process. The main issue is that Vulkan measures resolution in pixels, while GLFW, and many other window managers, use screen coordinates. In most cases, these will match with and correspond to each other. In some displays, however, they don't, and in those cases screen coordinates can correspond to a cluster of pixels rather than a single one.

One of the key "themes" of Vulkan is that extra work needs to be put in if you want to support every device, or every display. It's not too much work, but it's work nonetheless.

Fortunately, GLFW's ```glfwGetFramebufferSize``` will return the resolution of the window in pixels.

There's an additional detail - Vulkan usually sets an extent automatically in the ```currentExtent``` attribute of the ```VkSurfaceCapabilitiesKHR``` struct. *This is another reason why it's important that we've created the window surface by now.*

However, if the ```currentExtent``` width and height values are equal to the 32-bit unsigned integer limit, it's the window manager's way of telling us that we need to define this on our own. So, as the tutorial does, I'll only be defining the extent myself if the ```width``` member of the ```currentExtent``` matches this limit.
```
#include <algorithm> //std::clamp
#include <limits> //std::numeric_limits

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
```
This is pretty much ripped wholesale from the tutorial.

That's the preamble done. Now we can actually create the swap chain.
```
VkSwapchainKHR swapchain;
```
We can start by choosing all of those properties with those helper functions.
```
swap_chain_support supported_features = query_swap_chain_support(physical_device);

VkSurfaceFormatKHR sc_format = choose_swap_chain_format(supported_features);
VkPresentModeKHR sc_present_mode = choose_swap_chain_present_mode(supported_features);
VkExtent2D sc_extent = choose_swap_chain_extent(supported_features);
```
We'll also need to choose the amount of images we want in the swap chain. The recommended value to use is the minimum plus one, found with the ```minImageCount``` attribute of the ```VkSurfaceCapabilitiesKHR``` struct. I've added an additional check to make sure this value also doesn't exceed the maximum defined by ```maxImageCount``` (**note:** a maximum image of 0 indicates no maximum at all).
```
uint32_t sc_max_image_count = supported_features.capabilities.maxImageCount;

uint32_t sc_image_count = supported_features.capabilities.minImageCount + 1;
if(sc_image_count > sc_max_image_count && sc_max_image_count != 0)
    sc_image_count = sc_max_image_count;
```
The creation info struct required here is ```VkSwapchainCreateInfoKHR```.
```
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
```
Other than the last two attributes, these should be self-explanatory.

```imageArrayLayers``` determines the number of layers each image should have. In most cases this should be 1, but if you're developing a stereoscopic 3D application you may use multiple.

```imageUsage``` determines how the image will be used. We just want to render color information directly to the image to be displayed on screen, so we use ```VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT```.

The next property that needs to be set for the images is ```imageSharingMode```, which determines how images can be shared across different queues. There are two modes:

- ```VK_SHARING_MODE_EXCLUSIVE``` - An image can only be owned by a single queue family at a time. Ownership must be explicitly transferred before using it in another queue family. Best performing optoin.
- ```VK_SHARING_MODE_CONCURRENT``` - Images can be used across multiple queue families without ownership transfers. Most convenient option.

For the sake of simplicity, as the tutorial dictates, I'm only using ```VK_SHARING_MODE_EXCLUSIVE``` if the graphics and presentation queue families are one and the same.

```
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
```
There are a few other properties to set for the swap chain. ```preTransform``` allows you to apply a transformation, such as a 90 degree rotation, or horizontal flip, to each image prior to displaying in the swap chain. Supported transformations are found in ```VkSurfaceCapabilitiesKHR.supportedTransforms```. If you don't want to apply any transformations, you can just use ```VkSurfaceCapabilitiesKHR.currentTransform```.
```
info_swapchain.preTransform = supported_features.capabilities.currentTransform;
```
The ```compositeAlpha``` attribute specifies if the alpha channel can blend with other windows in the window system. In most cases, you'll just want to ignore the alpha channel in the final image, especially if the image is the one being displayed on screen, which is when you'd want to use ```VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR```.
```
info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
```
The ```clipped``` attribute specifies if pixels rendered in the front of the swap chain should replace pixels behind the swap chain. Once again, in most cases, the answer to this is yes.
```
info_swapchain.clipped = VK_TRUE;
```
Finally, there's a field for ```oldSwapchain```. We'll revisit this later. For now, I'm leaving it as ```VK_NULL_HANDLE```.
```
info_swapchain.oldSwapchain = VK_NULL_HANDLE;
```
At last, we can create the swap chain with ```vkCreateSwapchainKHR```.
```
VkResult result = vkCreateSwapchainKHR(device, &info_swapchain, nullptr, &swapchain);
```
Notice how the first parameter is the logical device, instead of the instance. That will be a rule going forward.

Once again, we have to destroy this resource when the program terminates.
```
vkDestroySwapchainKHR(device, swapchain, nullptr);
```
Whew, that was a lot. Don't worry, there are bigger things coming. For now, step 4 is completed. Only 4 more to go.
## Image Views
Step 5 consists of a bunch of little things we need to take care of before setting up the Vulkan graphics pipeline. The first of these is accessing the images from the newly created swap chain, and applying image views to each. An image view is simply a subset of an image, almost just a wrapper.

After the swap chain has been created, I've extended the creation function a little to grab the images, as well as store some additional information for other parts of the program to use.
```
//'Global' handles for swap chain stuff.

VkSurfaceFormatKHR swapchain_format;
VkExtent2D swapchain_extent;

std::vector<VkImage> swapchain_images;
std::vector<VkImageView> swapchain_image_views;

[...]
//Inside the swap chain creation function

swapchain_format = sc_format;
swapchain_extent = sc_extent;

vkGetSwapchainImagesKHR(device, swapchain, &sc_image_count, nullptr);
swapchain_images.resize(sc_image_count);
vkGetSwapchainImagesKHR(device, swapchain, &sc_image_count, swapchain_images.data());
```
We need one image view for each image, so figuring out how many image views we need from here should be trivial.

Each image view creation will also need a ```VkImageViewCreateInfo``` struct.
```
VkImageViewCreateInfo info_image_view{};
info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
info_image_view.image = swapchain_images[i];
info_image_view.format = swapchain_format.format;
```
There are a handful of noteworthy attributes to this struct. First of all, the ```viewType``` attribute allows you to specify how the image data should be interpreted, whether it's a 1D, 2D, or 3D texture, or a cube map.
```
info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
```
Then there are two structs contained within ```VkImageViewCreateInfo``` whose attributes also need to be defined. First, there's ```components```, a ```VkComponentMapping``` struct that essentially allow you to swizzle the color channels around, and can also be set to a constant value of either 1 or 0.
```
info_image_view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
info_image_view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
info_image_view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
info_image_view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```
The ```subresourceRange``` is a ```VkImageSubresourceRange```, and it's components generally describe the purpose of the image, and which part of the image should be accessed. In more specific terms, this means describing mipmap levels and image layers.
```
info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
info_image_view.subresourceRange.baseMipLevel = 0;
info_image_view.subresourceRange.levelCount = 1;
info_image_view.subresourceRange.baseArrayLayer = 0;
info_image_view.subresourceRange.layerCount = 1;
```
That's all the information we need to create each image view.
```
VkResult result = vkCreateImageView(device, &info_image_view, nullptr, &swapchain_image_views[i]);
```
Once again, deleting each image view is necessary.
```
for(uint32_t i = 0; i < swapchain_image_views.size(); i++)
{
    vkDestroyImageView(device, swapchain_image_views[i], nullptr);
}
```

## Render Passes
Render passes wrap information about framebuffer attachments, like color and depth buffers and how many of them there will be, how many samples will be needed for each of them (for multisampling), and how their contents should be handled throughout rendering operations.
```
VkRenderPass render_pass;
```
The first struct we need to make use of is a ```VkAttachmentDescription```. In this case, we only want a single color buffer attachment to the framebuffer, so only one of these attachments should be used.

```
VkAttachmentDescription attachment_color{};

attachment_color.format = swapchain_format.format;
attachment_color.samples = VK_SAMPLE_COUNT_1_BIT;
attachment_color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attachment_color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
attachment_color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
attachment_color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
attachment_color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
attachment_color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```
Since we're not doing multisampling yet, we set the number of ```samples``` to just 1. The ```format``` of the color attachment should match the format of the swap chain, since we're planning on displaying color images with the swap chain.

```loadOp``` and ```storeOp``` are where things get interesting - they determine what to do with the data before and after rendering. For ```loadOp```, the following choices are provided:

- ```VK_ATTACHMENT_LOAD_OP_LOAD```: Whatever pre-existing contents are in the attachment are loaded in for the next pass.
- ```VK_ATTACHMENT_LOAD_OP_CLEAR```: All values in the attachment are cleared (set) to a constant when being prepared for rendering.
- ```VK_ATTACHMENT_LOAD_OP_DONT_CARE```: Existing contents are treated as undefined - we don't care about them.

For ```storeOp```, there are only two choices:

- ```VK_ATTACHMENT_STORE_OP_STORE```: Rendered contents will be stored in the attachment and used later.
- ```VK_ATTACHMENT_STORE_OP_DONT_CARE```: Contents of the framebuffer attachment will be undefined after the rendering operation completes.

```loadOp``` and ```storeOp``` both apply to color and depth data, while ```stencilLoadOp``` and ```stencilStoreOp``` apply to stencil data. We won't use stencil buffers for this demo, so we treat these operations as irrelevant.

The ```initialLayout``` and ```finalLayout``` specify which layouts Vulkan should expect before the render pass begins, and the layout the image should automatically transition to when the render pass finishes, respectively. We don't care about the ```initialLayout``` of the image since all values will be cleared anyway in this example, but we do care about the ```finalLayout```. We want the image to be presented onto the screen after it's finished rendering, so we use ```VK_IMAGE_LAYOUT_PRESENT_SRC_KHR```.

Render passes also consist of one or more **subpasses**, which are post-render operations. The main purpose of using these is, once again, optimization. Vulkan has the capacity to reorder rendering operations and conserve memory bandwidth for some performance improvements.

Every subpass contains one or more references to attachments, which are defined with a ```VkAttachmentReference``` struct.
```
VkAttachmentReference attachment_color_reference{};

attachment_color_reference.attachment = 0;
attachment_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
```
The ```attachment``` parameter specifies the index of the attachment to reference, by it's index in the attachments array for the render pass. We only use one attachment, so its index is 0. The ```layout``` specifies the layout that the attachment should have for the subpass that uses the reference. Again, Vulkan will automatically transition to this layout when the subpass begins. ```VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL``` gives the best performing layout for color attachment subpasses.

After that's done, we can create a ```VkSubpassDescription``` to describe the actual subpass.
```
VkSubpassDescription subpass_color{};

subpass_color.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass_color.colorAttachmentCount = 1;
subpass_color.pColorAttachments = &attachment_color_reference;
```
The ```pipelineBindPoint``` may be able to specify if subpasses are used for compute operations instead of graphics operations if Vulkan supports it in the future, so that's why that field exists.

Now we can finally create the actual render pass.
```
VkRenderPassCreateInfo info_render_pass{};

info_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
info_render_pass.attachmentCount = 1;
info_render_pass.pAttachments = &attachment_color;
info_render_pass.subpassCount = 1;
info_render_pass.pSubpasses = &subpass_color;

VkResult result = vkCreateRenderPass(device, &info_render_pass, nullptr, &render_pass);
```
I trust most of these parameters are self-explanatory at this point. Like everything else, this handle will need to be destroyed eventually.
```
vkDestroyRenderPass(device, render_pass, nullptr);
```
## Framebuffers
Hey, look at that, we can finally set up the actual framebuffers now. Don't worry, this final part of Step 5 is very quick.
```
std::vector<VkFramebuffer> framebuffers;
```
Like the image views, we need one framebuffer for each image in the swap chain, and each framebuffer will also make use of their corresponding image view. Using the same approach as image views we can create each framebuffer through iteration:
```
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
    //Checking to make sure result is VK_SUCCESS.
}
```
By this point, all of the above parameters should be very self-explanatory, except for ```layers```, which depends on how many layers are in potential image arrays in the swap chain. We use single images, so just 1 suffices. These are destroyed just like image views:
```
for(uint32_t i = 0; i < framebuffers.size(); i++)
{
    vkDestroyFramebuffer(device, framebuffers[i], nullptr);
}
```
## The Vulkan Graphics Pipeline
It's worth getting an overview of what the individual steps are in the Vulkan graphics pipeline. It resembles very clearly the OpenGL graphics pipeline, so none of these concepts should be too new to you if you're not a beginner graphics programmer.

When vertex data is passed to the GPU, it's collected by an *input assembler*, which reads primitive data and assembles it into vertex data which is then passed to the GPU.

From here, each vertex is through a *vertex shader*, which can apply transformations or add data to each vertex, and then passes that data down to the next stages of the pipeline.

From there, two optional steps may be chosen. A *tessellation shader* can allow you to subdivide geometry, providing a mesh with more detail, and a *geometry shader* takes in primitives and discard or output more primitives than what went in.

After this, the meshes are then *rasterized*, which takes the vertices and the primitives which they make up, whether they be points, lines, or triangles, and fills in the pixels, or fragments (pixel elements). The attributes outputted by the previous shader stage are interpolated across these fragments.

Then, the *fragment shader* is run for each fragment drawn, determining how each fragment will be colored, and for what framebuffer they're being written to.

One final thing - *color blending* occurs after the fragment shader is done to mix different fragments that map to the same pixel in the framebuffer. In many cases, a fragment drawn later will just overwrite the fragment drawn earlier.

The *input assembler*, *rasterizer*, and *color blending* stages are *fixed-function* stages, meaning that they consist of predefined Vulkan behavior that the developer can tweak, but not directly change.

On the other hand, all of the shader stages are *programmable*, and consist of small programs you write to determine how they behave.

For the sake of this demo, both of the optional shader stages can be skipped.

## Shader Modules
Shader programs in Vulkan are compiled ahead-of-time, rather than just-in-time like with OpenGL, basically meaning that shader programs need to be precompiled into a bytecode format called SPIR-V before being used in a graphics pipeline. This leaves no room for drivers to reinterpret GLSL code like with OpenGL.

That being said, GLSL can still be used with Vulkan, and Khronos released their own vendor-independent compiler which will compile GLSL to SPIR-V. This compiler is contained in the SDK, called ```glslangValidator.exe```. There's also another compiler called ```glslc.exe```, created by none other than Google. This compiler is very similar to GCC and Clang, and it's the one I'll be using for this demo.

Here is the simple vertex shader program that I'm going to be using for this demo (```vertex.vert```):
```
#version 450
layout(location = 0) out vec3 f_color;

vec2 pos[3] = vec2[] (vec2(0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));

vec3 colors[3] = vec3[]
(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

void main()
{
    f_color = colors[gl_VertexIndex];
    gl_Position = vec4(pos[gl_VertexIndex], 0, 1);
}
```
Have to say, it's almost comforting seeing the ```gl``` prefix at this point.

To avoid having to go into creating vertex buffers for this demo, I've hardcoded the positions into the shader as the tutorial suggests. The ```gl_VertexIndex``` variable is a built-in variable which contains the current index of the current vertex, believe it or not.

And here's the fragment shader (```fragment.frag```):
```
#version 450
layout(location = 0) out vec4 color;

layout(location = 0) in vec3 f_color;

void main()
{
    color = vec4(f_color, 1);
}
```
These can be compiled, assuming ```glslc``` is in the system path, with the following commands:
```
glslc src/shaders/vertex.vert -o src/shaders/vertex.spv
glslc src/shaders/fragment.frag -o src/shaders/fragment.spv
```
**NOTE:** ```glslc``` seems to throw a "File Format Not Recognized" error if the extension of the file it's compiling is not ```.vert``` or ```.frag```, despite GLSL having no official file format extensions.

I'm not going to go into detail about the shaders right now since you should know what these do and mean if you have any experience with OpenGL or other graphics APIs.

These output files are bytecode files, and so they'll need to be read in as a string or array of bytes.

Then, they'll get represented with ```VkShaderModule``` objects. These shader modules require a ```VkShaderModuleCreateInfo``` struct for creation.
```
//'code' is an array of chars (8-bit ints), of length 'length'.

VkShaderModuleCreateInfo info_module{};
info_module.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
info_module.codeSize = length;
info_module.pCode = reinterpret_cast<const uint32_t*>(code);
```
And then the shader module can be created:
```
VkResult result = vkCreateShaderModule(device, &info_module, nullptr, &shader_module);
```
Cleaning these up can actually be done immediately after the graphics pipeline is created, so I'll hold off on that until the next part of this step.

## Creating the Pipeline
This is the biggest part of the biggest step of this journey. Let's get right into it.
```
VkPipeline pipeline;
```
This part of the creation process is going to be all about tuning and describing the fixed function stages of the Vulkan graphics pipeline. These include:
- Using shader modules.
- Dynamic states.
- Vertex input.
- Input assembly.
- The viewport.
- The rasterizer.
- Multisampling.
- Depth and stencil testing.
- Color blending.
- Overall pipeline layout.

Each of these pieces has it's own struct that needs to be defined and have its attributes set, with the exception of the depth and stencil testing piece.

### Using Shader Modules
To use the shaders loaded in the previous section, they need to be assigned a specific shader stage. Here is an example for a vertex shader module:
```
VkPipelineShaderStageCreateInfo info_shader[2];

info_shader[0] = {};
info_shader[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
info_shader[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
info_shader[0].module = vs_module;
info_shader[0].pName = "main";
```
All of these attributes besides ```pName``` should be fairly self-explanatory. ```pName``` refers to the identifier of the entrypoint function. In this case, I'm sticking with the standard "```main```", but this means it's possible to create multiple shader stages using the same shader module and multiple different entry points.

In this example, I have an array of two ```VkPipelineShaderStageCreateInfo```s, one for the vertex shader and one for the fragment shader.

### Dynamic States
In Vulkan, the idea of a "dynamic" graphics pipeline does not really exist - if you want your renderer to exhibit dynamic behavior, you would need to switch between different graphics pipelines, not change the one you're currently using.

That being said, there are some small portions of the Vulkan graphics pipeline that can be dynamic, such as the viewport size, line width, and color blending constants.
```
VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

VkPipelineDynamicStateCreateInfo info_dynamic_state{};
info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
info_dynamic_state.dynamicStateCount = 2;
info_dynamic_state.pDynamicStates = dynamic_states;
```
What this will do is cause the pipeline to ignore ahead-of-time configurations for these values, and you'll need to specify data at runtime.

### Vertex Input
The vertex input stage describes the vertex data that will be passed to the vertex shader, whether the data is per-vertex or per-instance, how many attributes will be passed, and which binding and offset each will load from.

Since the vertex data is currently being hard-coded into the shader, this structure currently specifies that no data will be loaded.
```
VkPipelineVertexInputStateCreateInfo info_vertex_input{};
    
info_vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
info_vertex_input.vertexBindingDescriptionCount = 0;
info_vertex_input.vertexAttributeDescriptionCount = 0;
```
### Input Assembly
The input assembly stage defines two things:

- The type of geometry that should be drawn.
- If primitive restart should be enabled (some value in an index array is specified that triggers the end of a primitive - most useful with strips).
```
VkPipelineInputAssemblyStateCreateInfo info_input_assembly{};

info_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
info_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
info_input_assembly.primitiveRestartEnable = VK_FALSE;
```
Here I go with a simple use case of a triangle list with no primitive restart.
### The Viewport
The viewport is defined using a ```VkViewport```, which stores information about and describes the region of the framebuffer which image output will be rendered to. Unless you plan on artificially increasing or lowering the resolution, this will almost always match the width and height of the window.
```
VkViewport viewport{};

viewport.x = 0;
viewport.y = 0;
viewport.width = (float) swapchain_extent.width;
viewport.height = (float) swapchain_extent.height;
viewport.minDepth = -1;
viewport.maxDepth = 1;
```
Note how the viewport also contains depth cutoffs built in.

Because Vulkan is just that verbose, we also need to specify a scissor rectangle, which is a subset of the viewport for which pixels will actually be drawn. This is useful for cases where part of the window is outside of the user's monitor - we obviously don't need to render to pixels which the user can't see.
```
VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapchain_extent;
```
With that done, we can create the ```VkPipelineViewportStateCreateInfo```.
```
VkPipelineViewportStateCreateInfo info_viewport{};

info_viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
info_viewport.viewportCount = 1;
info_viewport.pViewports = &viewport;
info_viewport.scissorCount = 1;
info_viewport.pScissors = &scissor;
```
Yes, it's possible to render to multiple viewports, but doing so requires enabling a GPU feature.
### The Rasterizer
The rasterizer takes the geometry output from the vertex shader and transforms it into a list of fragments to be colored by the fragment shader. It also performs depth testing, face culling, and the scissor-viewport test we described in the previous step.
```
VkPipelineRasterizationStateCreateInfo info_rasterizer{};

info_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
info_rasterizer.rasterizerDiscardEnable = VK_FALSE;
info_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
info_rasterizer.lineWidth = 1;
info_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
info_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
info_rasterizer.depthBiasEnable = VK_FALSE;
```
Here's what each attribute defines:

- ```rasterizerDiscardEnable```: If enabled, all geometry passed into the rasterizer is discarded, effectively disabling output to the framebuffer.
- ```polygonMode```: Determines how fragments are generated from triangles. The one most commonly used will likely be ```VK_POLYGON_MODE_FILL```, which just fills triangles in. Others that are available are ```VK_POLYGON_MODE_LINE``` and ```VK_POLYGON_MODE_POINT```, but using them would require enabling a GPU feature.
- ```lineWidth```: specifies the width of lines drawn (using ```VK_POLYGON_MODE_LINE```) if that polygon mode is enabled. Using any value greater than 1 would require the enabling of the ```wideLines``` GPU feature.
- ```cullMode```: specifies which faces of triangles ought to be ignored by the rasterizer. ```VK_CULL_MODE_BACK_BIT``` tells the renderer to ignore the back-facing sides of triangles.
- ```frontFace```: specifies what defines the front-face of a triangle. In this case, we define the front face of a triangle to be where the vertices are in clockwise order.
- ```depthBiasEnable```: if enabled, the rasterizer can alter the depth values produced by adding a constant to them or biasing them based on the "slope" of a fragment. Sometimes used for shadow mapping.

### Multisampling
Multisampling configures how multiple polygons that rasterize to the same pixel should be treated/resolved. This stage is often used to perform anti-aliasing.

Actually using multisampling requires enabling a GPU feature. By default, pixels just overwrite without aliasing.
```
VkPipelineMultisampleStateCreateInfo info_multisample{};

info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
info_multisample.sampleShadingEnable = VK_FALSE;
info_multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
```
### Depth and Stencil Testing
This is the only stage that we can safely skip for now, as we have no use for depth testing or stencil buffers with this simple demo.

### Color Blending
When a fragment shader has returned a color, it should be combined somehow with a color which has already been written to the framebuffer. Generally speaking, Vulkan provides two ways to perform color blending:

- Mix the old and new values somehow to produce a final color.
- Combine the old and new using bitwise operations.

There are two structs that we need to worry about for this stage: the expected ```VkPipelineColorBlendStateCreateInfo``` struct, as well as a list of ```VkPipelineColorBlendAttachmentState```s, which contain the color blending operation configurations *per-framebuffer*, while the former is used for global pipeline settings.

In my case, I'm not going to worry about semi-transparent (or completely transparent) fragments, and I'll instead just have the framebuffer overwrite pixels rather than try to blend them.
```
VkPipelineColorBlendAttachmentState attachment_color_blend{};

attachment_color_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
attachment_color_blend.blendEnable = VK_FALSE;

VkPipelineColorBlendStateCreateInfo info_color_blending{};

info_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
info_color_blending.logicOpEnable = VK_FALSE;
info_color_blending.attachmentCount = 1;
info_color_blending.pAttachments = &attachment_color_blend;
```
Since this is not really a satisfactory overview of how color blending works in Vulkan, I'll follow with the tutorial and dig a little deeper.

Fundamentally, Vulkan's color blending works with the following simple algortithm:
```
if(<blendEnable>)
{
    color.rgb = (<srcColorBlendFactor> * new_color.rgb) <colorBlendOp> (<dstColorBlendFactor> * current_color.rgb)
    color.a = (<srcAlphaBlendFactor> * new_color.a) <alphaBlendOp> (<dstAlphaBlendFactor> * current_color.a)
}
else color = new_color;
```
The variables surrounded by ```<>```s are what can be changed via the corresponding attribute in the ```VkPipelineColorBlendAttachmentState```. For example:
```
attachment_color_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
attachment_color_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
attachment_color_blend.colorBlendOp = VK_BLEND_OP_ADD;

attachment_color_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
attachment_color_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
attachment_color_blend.alphaBlendOp = VK_BLEND_OP_ADD;
```
corresponds to:
```
color.rgb = (1 * new_color.rgb) + (0 * current_color.rgb)
color.a = (1 * new_color.a) + (0 * current_color.a)
```
which does the same thing as disabling blending when ```blendEnable``` is set to ```VK_TRUE```.

The standard alpha blending algorithm, which most engines use for semi-transparent materials, works as follows:
```
color.rgb = new_color.a * new_color.rgb + (1 - new_color.a) * current_color.rgb;
color.a = new_color.a;
```
This can be accomplished with the following parameters:
```
attachment_color_blend.blendEnable = VK_TRUE;

attachment_color_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
attachment_color_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
attachment_color_blend.colorBlendOp = VK_BLEND_OP_ADD;

attachment_color_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
attachment_color_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
attachment_color_blend.alphaBlendOp = VK_BLEND_OP_ADD;
```
The ```VkPipelineColorBlendStateCreateInfo``` struct is where you can determine if you want to use bitwise operators, and also lets you set blend constants you can use to further augment the blending algorithm Vulkan uses in the pipeline. In my case, I have ```logicOpEnable``` set to ```VK_FALSE```, so no bitwise operations will be done here.
### The Pipeline Layout
The pipeline layout handle specifies uniform variables passed to the shader. We're not using any uniform variables for this demo, but specifying some pipeline layout is still required.
```
//'Global' handle for the graphics pipeline layout.
VkPipelineLayout pipeline_layout;

[...]

//Inside of the pipeline creation function...
VkPipelineLayoutCreateInfo info_pipeline_layout{};

info_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
info_pipeline_layout.setLayoutCount = 0; //Default value - optional.

VkResult result = vkCreatePipelineLayout(device, &info_pipeline_layout, nullptr, &pipeline_layout);
```
As is implied by the ```vkCreate...``` function, this handle also must be destroyed, probably just before the pipeline itself.
```
vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
```
### The Pipeline
We now have everything necessary to create the actual graphics pipeline handle. We have the shader stages, pipeline layout, and render passes that are going to be used. We've also specified every fixed-function stage that we're using or that is required.

Following the creation of the graphics pipeline, the shader modules can both be destroyed using ```vkDestroyShaderModule``` - holding them in memory will be unnessecary after the pipeline is created.
```
VkGraphicsPipelineCreateInfo info_pipeline{};

info_pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
info_pipeline.stageCount = 2;
info_pipeline.pStages = info_shader;
```
The ```stageCount``` and ```pStages``` fields point to shader stage information and modules.
```
info_pipeline.pVertexInputState = &info_vertex_input;
info_pipeline.pInputAssemblyState = &info_input_assembly;
info_pipeline.pViewportState = &info_viewport;
info_pipeline.pRasterizationState = &info_rasterizer;
info_pipeline.pMultisampleState = &info_multisample;
info_pipeline.pDepthStencilState = nullptr;
info_pipeline.pColorBlendState = &info_color_blending;
info_pipeline.pDynamicState = &info_dynamic_state;
```
Here are all of the fixed-function stage information structs we defined in this step. Notice how the ```pDepthStencilState``` is ```nullptr```. Then we attach the pipeline layout to this struct:
```
info_pipeline.layout = pipeline_layout;
```
Then we finally tell Vulkan where this graphics pipeline will be used: subpass 0 in the render pass we defined earlier:
```
info_pipeline.renderPass = render_pass;
info_pipeline.subpass = 0;
```
Some additional info: Vulkan lets you create other pipelines by deriving from existing ones, potentially speeding up the creation process. This can be done by specifying the ```VK_PIPELINE_CREATE_DERIVATIVE_BIT``` in the ```flags``` attribute of the ```VkGraphicsPipelineCreateInfo``` struct, and then specifying ```basePipelineHandle``` for an existing pipeline to derive from, or ```basePipelineIndex``` for another pipeline about to be created by index.
```
VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info_pipeline, nullptr, &pipeline);
```
Here is the deletion function:
```
vkDestroyPipeline(device, pipeline, nullptr);
```
The graphics pipeline should be deleted before the ```VkPipelineLayout``` and ```VkRenderPass``` objects.

We're at last done with Step 6.
## Command Buffers
Now that the graphics pipeline is created, we'll be revisiting some earlier tasks like queues and queue families to allocate some command buffers. Commands in Vulkan are not executed directly with function calls like OpenGL, instead, commands must be recorded into a command buffer and then submitted in bulk. This allows Vulkan to most efficiently process the commands, and also allows command recording to be multithreaded.

Before a ```VkCommandBuffer``` can be created, a ```VkCommandPool``` must be created to allocate a buffer from.
```
queue_family_indices qf_indices = find_physical_device_queue_families(physical_device);

VkCommandPoolCreateInfo info_create_pool{};

info_create_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
info_create_pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
info_create_pool.queueFamilyIndex = qf_indices.graphics_index.value();
```
The point of a command pool is to access commands from a queue family to allocate command buffers.

There are two flags for command pools:

- ```VK_COMMAND_POOL_CREATE_TRANSIENT_BIT``` - Hints that command buffers are rerecorded with new commands very often. (might change memory allocation behavior).
- ```VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT``` - Allows command buffers to be rerecorded individually. Without this flag, they'd all have to be reset together.

This demo records a new command buffer every frame, so the second flag is checked.

Command buffers are executed by submitting them to a ```VkQueue```, which we'll be retrieving in the next step. For now, let's allocate some command buffers.
```
VkCommandBufferAllocateInfo info_alloc_buffer{};

info_alloc_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
info_alloc_buffer.commandPool = command_pool;
info_alloc_buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
info_alloc_buffer.commandBufferCount = 1;
```
We only need one command buffer for graphics commands. We use the command pool we created earlier, and I'm only allocating one buffer. The ```level``` attribute specifies if you're using **primary** or **secondary** command buffers. Primary command buffers can be submitted to a queue, but cannot be called from other command buffers, while secondary command buffers cannot be submitted directly, but can be called from other primary command buffers.

I'm going to follow along with the tutorial and create a function which writes (records) commands to the command buffer.
```
VkCommandBufferBeginInfo info_begin{};

info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

VkResult result = vkBeginCommandBuffer(command_buffer, &info_begin);
```
Yes, you need a struct to being recording to a command buffer. To tell a command buffer to begin a render pass, we need a ```VkRenderPassBeginInfo```.
```
VkRenderPassBeginInfo info_begin_render_pass{};

info_begin_render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
info_begin_render_pass.renderPass = render_pass;
info_begin_render_pass.framebuffer = framebuffers[image_index];
info_begin_render_pass.renderArea.offset = {0, 0};
info_begin_render_pass.renderArea.extent = swapchain_extent;
```
During this stage, since we set the render pass load operation to clear a color at the beginning of the color buffer pass, we should define a clear color:
```
VkClearValue clear_color = {{{0, 0, 0, 1}}};

info_begin_render_pass.clearValueCount = 1;
info_begin_render_pass.pClearValues = &clear_color;
```
Now we add the begin render pass command:
```
vkCmdBeginRenderPass(command_buffer, &info_begin_render_pass, VK_SUBPASS_CONTENTS_INLINE);
```
The last parameter defines how the drawing commands within the render pass will be provided. The ```VK_SUBPASS_CONTENTS_INLINE``` macro tells Vulkan that the render pass commands will be embedded in the primary command buffer itself, and no secondary command buffers will be executed. ```VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS``` tells Vulkan that the commands will be executed from secondary command buffers.

After this we can immediately bind the pipeline:
```
vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
```
Since the viewport and scissor rectangles were both specified as *dynamic states*, they need to be set each frame:
```
VkViewport viewport{};
viewport.x = 0;
viewport.y = 0;
viewport.width = (float) swapchain_extent.width;
viewport.height = (float) swapchain_extent.height;
viewport.minDepth = -1;
viewport.maxDepth = 1;

vkCmdSetViewport(command_buffer, 0, 1, &viewport);

VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapchain_extent;

vkCmdSetScissor(command_buffer, 0, 1, &scissor);
```
From there, we can perform the draw command, to draw the triangle baked into the vertex shader:
```
vkCmdDraw(command_buffer, 3, 1, 0, 0);
```
The first parameter after the command buffer specifies the number of vertices, the second parameter is the number of instances, the third is the first vertex to start with, and the fourth is the first instance to start with.

We're not doing instanced rendering, so we'll just use 1 and 0 for the second and fourth parameters.

That's all, we can end the render pass after this.
```
vkCmdEndRenderPass(command_buffer);
```
Then we can stop recording the command buffer.
```
VkResult result = vkEndCommandBuffer(command_buffer);
```
While command buffers don't need to be destroyed manually, command pools do. When command pools are destroyed, command buffers are automatically deallocated:
```
vkDestroyCommandPool(device, command_pool, nullptr);
```
All done with step 7, one more to go.
## Synchronization
Before we write the draw loop, there's an important Vulkan concept that must be understood:

By design, most Vulkan API calls are asynchronous, functions will return before the operation is finished. Synchronization of execution is explicit, and we need to explicitly tell Vulkan to wait until an image is acquired from the swap chain before executing commands that draw onto it, and we need to wait for those commands to finish before being presented to the screen.

Vulkan provides two synchronization primitives to help out: **semaphores**, and **fences**.

A **semaphore** adds order to queue operations, which refer to work submitted to a queue (typically command buffers). There are two kinds of semaphores in Vulkan: *binary*, and *timeline*. For this demo, only binary semaphores will be used.

Binary semaphores have two states: signaled, and unsignaled. All binary semaphores start out as unsignaled, and will become signaled when their associated work is completed. Other work for Vulkan queues can be set to wait on semaphores to be signaled to begin.

A **fence** is similar, but it's designed for synchronizing CPU execution with GPU commands, rather than syncing GPU commands with each other. Fences are similar to binary semaphores, in that they can either be signaled or unsignaled, and they'll start unsignaled. Submitted work can have a fence attached to it, and the fence will be signaled upon completion.

It's generally preferable to use semaphores rather than fences unless necessary, as fences block host execution. Fences also have to be reset manually.

Semaphores are created with a ```VkSemaphore``` handle, while fences use a ```VkFence```.
```
VkSemaphore sp_image_retrieved, sp_render_finished;
VkFence fence_frame_finished;
```
Creating them is rather simple:
```
VkSemaphoreCreateInfo info_create_semaphore{};
info_create_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

VkFenceCreateInfo info_create_fence{};
info_create_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

VkResult result = vkCreateSemaphore(device, &info_create_semaphore, nullptr, &sp_image_retrieved);
[...]
result = vkCreateSemaphore(device, &info_create_semaphore, nullptr, &sp_render_finished);
[...]
result = vkCreateFence(device, &info_create_fence, nullptr, &fence_frame_finished);
```
Destroying them is rather simple, too:
```
vkDestroySemaphore(device, sp_image_retrieved, nullptr);
vkDestroySemaphore(device, sp_render_finished, nullptr);
vkDestroyFence(device, fence_frame_finished, nullptr);
```
For reasons that will become clear in the next section, we actually need the frame fence to start signaled. Adding this to the ```VkFenceCreateInfo``` will take care of that for us.
```
info_create_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
```
## The Draw Loop
To start, we need to get access to both the graphics and presentation queues we checked for back in step 3. These queues are stored in ```VkQueue``` handles, and can be retrieved with ```vkGetDeviceQueue```.
```
//'Global' handles.
VkQueue queue_graphics, queue_present;

[...]

//Inside the main function.
queue_family_indices qf_indices = find_physical_device_queue_families(physical_device);

vkGetDeviceQueue(device, qf_indices.graphics_index.value(), 0, &queue_graphics);
vkGetDeviceQueue(device, qf_indices.presentation_index.value(), 0, &queue_present);
```
That third parameter represents the index of the queue handle we want to use. We can just use the first one available.

The first thing to do in the draw loop is wait for all previous frame operations to finish.
```
vkWaitForFences(device, 1, &fence_frame_finished, VK_TRUE, UINT64_MAX);
vkResetFences(device, 1, &fence_frame_finished);
```
After waiting has finished (i.e. when ```fence_frame_finished``` is signaled), CPU execution will resume, and the fence will be reset with ```vkResetFences```. This is why we start with ```fence_frame_finished``` signaled - if it was unsignaled the loop would remain here forever on the first frame.

Then, we acquire the next available image from the swap chain.
```
uint32_t image_index;
vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, sp_image_retrieved, VK_NULL_HANDLE, &image_index);
```
The fourth and fifth parameters are semaphores and fences to signal when the acquisition is finished, respectively. We use the ```sp_image_retrieved``` semaphore here. From here, we can go ahead and do some other things which don't require the image to be retrieved, such as resetting, then recording the command buffer:
```
vkResetCommandBuffer(command_buffer, 0);
record_command_buffer(command_buffer, image_index);
```
Now we can prepare the command buffer before submission using the ```VkSubmitInfo``` struct.
```
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
```
Here we specify the command buffer we're submitting to the queue (```pCommandBuffers```), semaphores we wait for before submitting (```pWaitSempahores```), and semaphores to signal once operations have completed (```pSignalSemaphores```).

Now, we can submit the commands to the graphics queue:
```
VkResult result = vkQueueSubmit(queue_graphics, 1, &info_submit, fence_frame_finished);
```
That's all for rendering! Now, we still need to present the image to the screen with the presentation queue, and we also need to make one tiny little addition to the render pass: a subpass dependency.
```
VkSubpassDependency subpass_dependency{};

subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
subpass_dependency.dstSubpass = 0;
subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
subpass_dependency.srcAccessMask = 0;
subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```
You know how subpasses automatically take care of image layout conversions/transitions? Said transitions are controlled with subpass dependencies, which specify memory and execution dependencies between subpasses, as the tutorial states.

There are two default transition dependencies, one which occurs at the start of the render pass and one at the end, but the former assumes the transition occurs at the start of the pipeline, which happens to be before image acquisition is ensured, which is why we specify one here. An alternate solution is to change the ```wait_stages``` to ```VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT```, which ensures that render passes don't begin until the image is available. This solution is more generalized, making render passes wait until for the ```VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT``` stage.

The ```srcSubpass``` specifies the index of the dependency. We use ```VK_SUBPASS_EXTERNAL``` to refer to the implicit subpass before or after the render pass depending on whether it's used in ```srcSubpass``` or ```dstSubpass``` respectively. The ```dstSubpass``` is our subpass, which is at index 0 being the first and only subpass in our render pass.

The ```srcStageMask``` and ```srcAccessMask``` specify the operations to wait on, as well as the stages in which the operations occur. In this case, we can set ```srcAccessMask``` to 0 to tell the render pass that we need to wait on the color output stage itself. The operations that should wait on the subpass are defined with ```dstStageMask``` and ```dstAccessMask```.

We can then add this dependency to the ```VkRenderPassCreateInfo``` struct and carry on:
```
info_render_pass.dependencyCount = 1;
info_render_pass.pDependencies = &subpass_dependency;
```
Back in the draw loop, we use a ```VkPresentModeKHR``` to specify details about swap chain image presentation:
```
VkPresentInfoKHR info_present{};
info_present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

info_present.waitSemaphoreCount = 1;
info_present.pWaitSemaphores = signal_semaphores;
```
In our case, we just want to make sure we wait on the signal semaphores listed earlier. We also want to add the actual swap chain to it:
```
VkSwapchainKHR swapchains[] = {swapchain};

info_present.swapchainCount = 1;
info_present.pSwapchains = swapchains;
info_present.pImageIndices = &image_index;
```
Then, we can simply call ```vkQueuePresentKHR```.
```
vkQueuePresentKHR(queue_present, &info_present);
```
This should finish presentation, and thus finish this demo. Running it now should present a triangle. There's one little problem - closing the window crashes the program, since cleanup will likely happen during still executing graphics/presentation operations on the GPU.

This can be fixed with a single line added after the draw loop:
```
vkDeviceWaitIdle(device);
```
This function waits for a logical device to finish operations before CPU execution resumes.
## Conclusion
There is still a bit more work to be done. Following the tutorial, some improvements can be made to the draw loop. As we currently have it, only one frame can be rendered to at a time, which kind of defeats the purpose of double-buffering and having a swap chain to begin with.

Also, this program should actually make use of it's dynamic states, changing the viewport and recreating the swap chain when the window gets resized.

Before I do any of that, though, I would like to take a look at what I have so far, and attempt to abstract it into a more organized and readable project, rather than just pack everything into a single source file. That, alongside the draw loop improvements, will be next on the list. For now, I'm willing to have this concluded as my first Vulkan experiment.

Like I said, these notes are ***heavily derivative*** of the tutorial, go read [that](https://docs.vulkan.org/tutorial/latest/00_Introduction.html) instead. These notes are meant to be more "proof of work", and to make sure I know that I understand the process myself.