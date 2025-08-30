#ifndef _MESH_H_
#define _MESH_H_

#include "alloc.h"

#include "../renderer/cmdbuffer.h"
#include "../renderer/pipeline.h"

class mesh
{
    public:
        mesh(pipeline_vertex_input pvi);
        ~mesh();

        void add_index(uint32_t index);
        void add_indices(std::vector<uint32_t> indices);

        bool add_vertex(std::vector<float> vertex);

        bool build();

        void draw(command_buffer* cmd_buffer);
    private:
        void clear_buffers();

        alloc::buffer vertex_buffer;
        alloc::buffer index_buffer;

        std::vector<float> data_vertices;
        uint16_t data_per_vertex;

        std::vector<uint32_t> data_indices;

        size_t num_indices;
        bool vb_created, ib_created, built;
};

#endif