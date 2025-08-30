cmake_minimum_required(VERSION 3.10)

project(VulkanTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

set(CPP_EXTENSION ".cpp")

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/)

set(BUILD_SOURCES
	"main"
	"renderer/vksetup"
	"renderer/swapchain"
	"renderer/renderpass"
	"renderer/pipeline"
	"renderer/cmdbuffer"
	"renderer/descriptor"
	"renderer/texture"
	"utils/alloc"
	"utils/vksync"
	"utils/linalg"
	"utils/image_utils"
	"utils/mesh"
	"utils/camera"
)

list(TRANSFORM BUILD_SOURCES APPEND ${CPP_EXTENSION})
list(TRANSFORM BUILD_SOURCES PREPEND ${SOURCE_DIR})

option(RELEASE "Create a release build." OFF)

set(LIBS "${CMAKE_SOURCE_DIR}/bin/glfw3.dll" "C:/Windows/System32/vulkan-1.dll")

# link_directories(${CMAKE_SOURCE_DIR}/bin "C:/Windows/System32")

add_executable(test ${BUILD_SOURCES})

if(NOT RELEASE)
	target_compile_definitions(test PRIVATE DEBUG_PRINT_SUCCESS)
else()
	add_compile_options(-g -Wall)
endif()

target_include_directories(test PRIVATE "${CMAKE_BINARY_DIR}/../../lib")
target_link_libraries(test ${LIBS})