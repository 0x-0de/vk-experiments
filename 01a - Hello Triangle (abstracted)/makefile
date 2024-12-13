GCC = g++
CPP_FLAGS = -g -Wall -std=c++17

GLSLC = glslc

DIR_EXP = bin
DIR_OBJ = bin/obj
DIR_OBJ_DBG = bin/obj/dbg
DIR_SRC = src
DIR_SHD = src/shaders

DEBUG_DEF = -DBUILD_DEBUG

LIBS = C:/Windows/System32/vulkan-1.dll -Lbin -lglfw3

OBJ = $(addprefix $(DIR_OBJ)/, main.o $(addprefix rendering/, vksetup.o swapchain.o renderpass.o pipeline.o vkcommands.o))
OBJ_DEBUG = $(addprefix $(DIR_OBJ_DBG)/, main.o $(addprefix rendering/, vksetup.o swapchain.o renderpass.o pipeline.o vkcommands.o))

OBJ_VERTEX_SHADER = $(addprefix $(DIR_SHD)/, vertex.spv)
OBJ_FRAGMENT_SHADER = $(addprefix $(DIR_SHD)/, fragment.spv)

debug: $(OBJ_DEBUG)
	$(GCC) $(OBJ_DEBUG) $(LIBS) -o $(DIR_EXP)/test.exe

release: $(OBJ)
	$(GCC) $(OBJ) $(LIBS) -o $(DIR_EXP)/test.exe

run:
	$(DIR_EXP)/test.exe

shaders: $(OBJ_VERTEX_SHADER) $(OBJ_FRAGMENT_SHADER)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.cpp | $(DIR_OBJ)
	$(GCC) $(CPP_FLAGS) -c $< -o $@

$(DIR_OBJ_DBG)/%.o: $(DIR_SRC)/%.cpp | $(DIR_OBJ_DBG)
	$(GCC) $(CPP_FLAGS) $(DEBUG_DEF) -c $< -o $@

#$(OBJ_VERTEX_SHADER): $(DIR_SHD)/%.vert
#	$(GLSLC) $< -o $@

clean:
	rm -rf $(DIR_OBJ)
	rm $(DIR_EXP)/test.exe

$(DIR_OBJ):
	mkdir -p $@
	mkdir -p $@/rendering

$(DIR_OBJ_DBG):
	mkdir -p $@
	mkdir -p $@/rendering