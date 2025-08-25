#include "mesh.h"

#include <iostream>

mesh::mesh(pipeline_vertex_input pvi)
{
    data_per_vertex = pvi.vertex_binding.stride / sizeof(float);

    vb_created = false;
    ib_created = false;

    built = false;
}

mesh::~mesh()
{
    clear_buffers();
}

void mesh::add_index(uint16_t index)
{
    data_indices.push_back(index);
}

void mesh::add_indices(std::vector<uint16_t> indices)
{
    for(size_t i = 0; i < indices.size(); i++)
        data_indices.push_back(indices[i]);
}

bool mesh::add_vertex(std::vector<float> vertex)
{
    if(vertex.size() != data_per_vertex)
    {
        std::cerr << "[VK|ERR] Attempted to add invalid vertex (size " << vertex.size() << ") to mesh with vertex size " << data_per_vertex << "." << std::endl;
        return false;
    }

    for(size_t i = 0; i < vertex.size(); i++)
        data_vertices.push_back(vertex[i]);

    return true;
}

bool mesh::build()
{
    clear_buffers();

    if(!alloc::new_buffer(&vertex_buffer, data_vertices.data(), data_vertices.size() * sizeof(float), ALLOC_USAGE_STAGED_VERTEX_BUFFER)) return false;
    vb_created = true;

    if(!alloc::new_buffer(&index_buffer, data_indices.data(), data_indices.size() * sizeof(uint16_t), ALLOC_USAGE_STAGED_INDEX_BUFFER)) return false;
    ib_created = true;

    num_indices = data_indices.size();

    std::vector<float>().swap(data_vertices);
    std::vector<uint16_t>().swap(data_indices);

    built = true;
    return built;
}

void mesh::clear_buffers()
{
    if(vb_created) alloc::free(vertex_buffer);
    if(ib_created) alloc::free(index_buffer);

    vb_created = false;
    ib_created = false;

    built = false;
}

void mesh::draw(command_buffer* cmd_buffer)
{
    cmd_buffer->bind_vertex_buffer(vertex_buffer.vk_buffer, 0);
    cmd_buffer->bind_index_buffer(index_buffer.vk_buffer, 0);

    if(built) cmd_buffer->draw_indexed(num_indices, 1);
}