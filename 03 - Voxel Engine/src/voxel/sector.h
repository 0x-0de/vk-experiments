#ifndef _SECTOR_H_
#define _SECTOR_H_

#include "../renderer/descriptor.h"

#include "../utils/mesh.h"

#define SECTOR_FACTOR 6
#define SECTOR_SIZE (1<<SECTOR_FACTOR)

class sector
{
	public:
		sector(int64_t x, int64_t y, int64_t z);
		~sector();
		
		void build();
		
		void draw(command_buffer* cmd_buffer, descriptor* uniforms, uint8_t frame_index);
		
		bool is_facing(char*** facing, uint16_t x, uint16_t y, uint16_t z, uint8_t face);
		
		void generate();
		
		static void init(uint64_t seed);
		
		void set(uint16_t x, uint16_t y, uint16_t z, uint32_t value, bool reload);
	private:
		int64_t x, y, z;
		bool can_draw;
		
		mesh* m;
		uint32_t*** voxels;
		
		float transform_data[16];
};

#endif