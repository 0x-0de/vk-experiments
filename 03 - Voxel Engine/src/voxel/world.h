#ifndef _WORLD_H_
#define _WORLD_H_

#include "../../lib/GLFW/glfw3.h"

#include "../renderer/cmdbuffer.h"

#include "../utils/camera.h"

namespace world
{
    void deinit();

    void draw(command_buffer* cmd_buffer, pipeline* pl);

    size_t get_sector_index(int64_t x, int64_t y, int64_t z);

    void init();

    void update_sectors_main_thread(camera3d* camera);
    void update_sectors_alt_thread();

    void update_input(GLFWwindow* window, bool window_focused, uint32_t* voxel_selection_data);
}

#endif