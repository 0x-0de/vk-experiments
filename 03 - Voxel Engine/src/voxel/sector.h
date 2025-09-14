#ifndef _SECTOR_H_
#define _SECTOR_H_

#include "../renderer/descriptor.h"

#include "../utils/mesh.h"

#define SECTOR_FACTOR 6
#define SECTOR_SIZE (1<<SECTOR_FACTOR)

#define SECTOR_STATE_NEW 0
#define SECTOR_STATE_GENERATED 1
#define SECTOR_STATE_DRAWABLE 2
#define SECTOR_STATE_EMPTY 3
#define SECTOR_STATE_MESH_LOADED 4

class sector
{
	public:
		sector(int64_t x, int64_t y, int64_t z);
		~sector();
		
		void build();
		
		void draw(command_buffer* cmd_buffer);
		
		bool is_facing(char*** facing, uint16_t x, uint16_t y, uint16_t z, uint8_t face);
		
		void generate();
		void get_pos(int64_t* pos_x, int64_t* pos_y, int64_t* pos_z);

		uint8_t get_state() const;
		
		static void init(uint64_t seed);

		void load_mesh();
		
		void set(uint16_t x, uint16_t y, uint16_t z, uint32_t value, bool reload);
	private:
		int64_t x, y, z;
		uint8_t state;
		
		mesh* m;
		uint32_t*** voxels;
		
		float transform_data[16];
};

#endif