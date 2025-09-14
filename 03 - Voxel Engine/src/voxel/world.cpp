#include "world.h"

#include "../ref.h"

#include <cmath>
#include <vector>

#include "sector.h"
#include "../utils/linalg.h"

#define SECTOR_LAYER_SIZE 3

struct sector_draw_data
{
    float transform_data[16];
    float sector_id;

    bool inside_bound_check;
};

static math::mat default_rot;

static std::vector<std::vector<sector*> > sectors;
static std::vector<std::vector<sector_draw_data> > sectors_pc_data;

static int voxel_timer = 0;

#define WORLD_PROCESS_MANAGING_SECTORS 0
#define WORLD_PROCESS_GENERATIING_SECTORS 1

static int current_process;

void world::deinit()
{
    for(size_t i = 0; i < sectors.size(); i++)
    {
        for(size_t j = 0; j < sectors[i].size(); j++)
        {
            delete sectors[i][j];
        }
        sectors[i].clear();
    }
    sectors.clear();
}

void world::draw(command_buffer* cmd_buffer, pipeline* pl)
{
    for(int i = 0; i < sectors.size(); i++)
    {
        for(int j = 0; j < sectors[i].size(); j++)
        {
            cmd_buffer->push_constants(pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), sectors_pc_data[i][j].transform_data);
			cmd_buffer->push_constants(pl, VK_SHADER_STAGE_VERTEX_BIT, 16 * sizeof(float), sizeof(float), &sectors_pc_data[i][j].sector_id);

            sectors[i][j]->draw(cmd_buffer);
            sectors_pc_data[i][j].inside_bound_check = false;
        }
    }
}

size_t world::get_sector_index(int64_t x, int64_t y, int64_t z)
{
    for(size_t i = 0; i < sectors[0].size(); i++)
    {
        int64_t sx, sy, sz;
        sectors[0][i]->get_pos(&sx, &sy, &sz);
        if(sx == x && sy == y && sz == z) return i;
    }

    return SIZE_MAX;
}

void world::init()
{
    sector::init(glfwGetTime() * 1000000000);
    default_rot = math::rotation(math::vec3(0, 0, 0));

    sectors.resize(1);
    sectors_pc_data.resize(1);

    current_process = WORLD_PROCESS_MANAGING_SECTORS;
}

void world::update_input(GLFWwindow* window, bool window_focused, uint32_t* voxel_selection_data)
{
    int voxel_x = ((int) voxel_selection_data[0]) >> 16;
    int voxel_y = (((int) voxel_selection_data[0]) >> 8) & 255;
    int voxel_z = ((int) voxel_selection_data[0]) & 255;
    
    int face = (int) voxel_selection_data[1];
    
    //std::cout << voxel_x << ", " << voxel_y << ", " << voxel_z << " : " << face << std::endl;
    
    if(voxel_timer > 0)
        voxel_timer--;

    if(window_focused && voxel_timer == 0 && voxel_selection_data[3] == 1)
    {
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
        {
            switch(face)
            {
                case 0:
                case 2:
                case 4:
                    break;
                case 1:
                    voxel_x -= 1;
                    break;
                case 3:
                    voxel_y -= 1;
                    break;
                case 5:
                    voxel_z -= 1;
                    break;
            }
            
            sectors[0][voxel_selection_data[2]]->set(voxel_x, voxel_y, voxel_z, 0, true);
        }
        
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
        {
            switch(face)
            {
                case 1:
                case 3:
                case 5:
                    break;
                case 0:
                    voxel_x -= 1;
                    break;
                case 2:
                    voxel_y -= 1;
                    break;
                case 4:
                    voxel_z -= 1;
                    break;
            }
            
            sectors[0][voxel_selection_data[2]]->set(voxel_x, voxel_y, voxel_z, 1, true);
        }
        
        voxel_timer = 25;
    }
}

void world::update_sectors_alt_thread()
{
    if(current_process == WORLD_PROCESS_GENERATIING_SECTORS)
    {
        for(int i = 0; i < sectors[0].size(); i++)
        {
            if(sectors[0][i]->get_state() == SECTOR_STATE_NEW)
            {
                //std::string log = "Generating: " + std::to_string(i + 1) + "/" + std::to_string(sectors[0].size());
                //INFO_LOG(log);
                
                sectors[0][i]->generate();
            }

            if(sectors[0][i]->get_state() == SECTOR_STATE_GENERATED)
            {
                //std::string log = "Loading: " + std::to_string(i + 1) + "/" + std::to_string(sectors[0].size());
                //INFO_LOG(log);

                sectors[0][i]->load_mesh();
            }
        }

        current_process = WORLD_PROCESS_MANAGING_SECTORS;
    }
}

void world::update_sectors_main_thread(camera3d* camera)
{
    if(current_process == WORLD_PROCESS_MANAGING_SECTORS)
    {
        math::vec camera_pos = camera->get_pos();

        int64_t cam_pos_x = std::floor(camera_pos[0] / SECTOR_SIZE);
        int64_t cam_pos_y = std::floor(camera_pos[1] / SECTOR_SIZE);
        int64_t cam_pos_z = std::floor(camera_pos[2] / SECTOR_SIZE);

        for(int64_t i = cam_pos_x - SECTOR_LAYER_SIZE; i <= cam_pos_x + SECTOR_LAYER_SIZE; i++)
        for(int64_t j = cam_pos_y - SECTOR_LAYER_SIZE; j <= cam_pos_y + SECTOR_LAYER_SIZE; j++)
        for(int64_t k = cam_pos_z - SECTOR_LAYER_SIZE; k <= cam_pos_z + SECTOR_LAYER_SIZE; k++)
        {
            size_t index = get_sector_index(i, j, k);
            if(index == SIZE_MAX)
            {
                sector* sec = new sector(i, j, k);
                sectors[0].push_back(sec);

                sector_draw_data scd;
            
                math::mat transform = math::transform(math::vec3(i, j, k) * SECTOR_SIZE, default_rot, math::vec3(1, 1, 1));
                transform.get_data(scd.transform_data);
                
                scd.sector_id = sectors[0].size() - 1;
                scd.inside_bound_check = true;

                sectors_pc_data[0].push_back(scd);

                //std::string str = "Created: " + std::to_string(i) + ", " + std::to_string(j) + ", " + std::to_string(k);
                //INFO_LOG(str);
            }
            else
            {
                sectors_pc_data[0][index].inside_bound_check = true;
            }
        }

        for(size_t i = 0; i < sectors.size(); i++)
        for(size_t j = 0; j < sectors[i].size(); j++)
        {
            if(!sectors_pc_data[i][j].inside_bound_check)
            {
                vkDeviceWaitIdle(get_device());
                
                delete sectors[i][j];
                sectors[i].erase(sectors[i].begin() + j);
                sectors_pc_data[i].erase(sectors_pc_data[i].begin() + j);

                for(size_t k = j; k < sectors_pc_data[i].size(); k++)
                {
                    sectors_pc_data[i][k].sector_id--;
                }

                j--;
            }
        }

        current_process = WORLD_PROCESS_GENERATIING_SECTORS;
    }

    if(current_process == WORLD_PROCESS_GENERATIING_SECTORS)
    {
        for(int i = 0; i < sectors[0].size(); i++)
        {
            if(sectors[0][i]->get_state() == SECTOR_STATE_MESH_LOADED)
            {
                //std::string log = "Building: " + std::to_string(i + 1) + "/" + std::to_string(sectors[0].size());
                //INFO_LOG(log);

                sectors[0][i]->build();
            }
        }
    }
}