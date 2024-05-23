CC = g++
CPPFLAGS = -std=c++17 -Wall -c -g

LIBDIR = -L./
LIBS = -lglfw3

DIRECT_LIBS = C:/Windows/System32/vulkan-1.dll

OBJLIST = main.o
SHADERS_LIST = main_vs.spv main_fs.spv

default: $(OBJLIST)
	$(CC) $(OBJLIST) $(DIRECT_LIBS) $(LIBDIR) $(LIBS) -o "Vulkan Test.exe"

shaders: $(SHADERS_LIST)

main.o: ../src/main.cpp
	$(CC) $(CPPFLAGS) -DAPP_DEBUG ../src/main.cpp

main_vs.spv: ../shd/main_vs.vert
	C:/VulkanSDK/1.3.280.0/Bin/glslc.exe ../shd/main_vs.vert -o ../src/shaders/main_vs.spv

main_fs.spv: ../shd/main_fs.frag
	C:/VulkanSDK/1.3.280.0/Bin/glslc.exe ../shd/main_fs.frag -o ../src/shaders/main_fs.spv

clean:
	-rm *.o