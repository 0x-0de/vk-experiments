#include "sector.h"

#define SECTOR_GEN_OPTIMIZE
#define SECTOR_GEN_OPTIMIZE_LEAP 4

#define FACE_LEFT 0
#define FACE_RIGHT 1
#define FACE_BOTTOM 2
#define FACE_TOP 3
#define FACE_FRONT 4
#define FACE_BACK 5
#define NUM_FACES 6

#include "../utils/linalg.h"

#include <cmath>

static pipeline_vertex_input pvi;

static uint64_t world_seed;

void sector::init(uint64_t seed)
{
	pvi.vertex_binding = create_vertex_input_binding(0, 8 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	pvi.vertex_attribs.resize(3);
	pvi.vertex_attribs[0] = create_vertex_input_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	pvi.vertex_attribs[1] = create_vertex_input_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
	pvi.vertex_attribs[2] = create_vertex_input_attribute(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float));
	
	world_seed = seed;
}

double generate_landscape(double x, double y, double z)
{
	double val = math::gradient_noise_3d_cosine(world_seed, x / 60, y / 30, z / 60);
	val += (double) ((signed) y - SECTOR_SIZE / 2) / 60;
	return val;
}

uint32_t get_voxel_code(uint16_t x, uint16_t y, uint16_t z)
{
	return (x << (SECTOR_FACTOR << 1)) | (y << SECTOR_FACTOR) | z;
}

void get_voxel_from_code(uint32_t code, uint16_t* x, uint16_t* y, uint16_t* z)
{
	uint16_t bound = ((1 << SECTOR_FACTOR) - 1);
	
	*x = code >> (SECTOR_FACTOR << 1);
	*y = (code >> SECTOR_FACTOR) & bound;
	*z = code & bound;
}

sector::sector(int64_t x, int64_t y, int64_t z) : x(x), y(y), z(z)
{
	voxels = new uint32_t**[SECTOR_SIZE];
	for(size_t i = 0; i < SECTOR_SIZE; i++)
	{
		voxels[i] = new uint32_t*[SECTOR_SIZE];
		for(size_t j = 0; j < SECTOR_SIZE; j++)
			voxels[i][j] = new uint32_t[SECTOR_SIZE];
	}
	
	m = new mesh(pvi);
	state = SECTOR_STATE_NEW;
	
	math::mat t = math::transform(math::vec3(x, y, z) * SECTOR_SIZE, math::rotation(math::vec3(0, 0, 0)), math::vec3(1, 1, 1));
	t.get_data(transform_data);
}

sector::~sector()
{
	delete m;
	
	for(size_t i = 0; i < SECTOR_SIZE; i++)
	{
		for(size_t j = 0; j < SECTOR_SIZE; j++)
			delete[] voxels[i][j];
		delete[] voxels[i];
	}
	delete[] voxels;
}

void sector::build()
{	
	state = m->build() ? SECTOR_STATE_DRAWABLE : SECTOR_STATE_EMPTY;
}

bool sector::is_facing(char*** facing, uint16_t x, uint16_t y, uint16_t z, uint8_t face)
{
	return (facing[x][y][z] & (1 << face)) == (1 << face);
}

void sector::draw(command_buffer* cmd_buffer)
{
	if(state == SECTOR_STATE_DRAWABLE) m->draw(cmd_buffer);
}

void sector::generate()
{
#ifdef SECTOR_GEN_OPTIMIZE
	uint32_t size = SECTOR_SIZE / SECTOR_GEN_OPTIMIZE_LEAP + 1;

	double*** gradient_values = new double**[size];
	for(size_t i = 0; i < size; i++)
	{
		gradient_values[i] = new double*[size];
		for(size_t j = 0; j < size; j++)
			gradient_values[i][j] = new double[size];
	}

	for(uint32_t i = 0; i < size; i++)
	for(uint32_t j = 0; j < size; j++)
	for(uint32_t k = 0; k < size; k++)
	{
		double pos_x = x * SECTOR_SIZE + i * SECTOR_GEN_OPTIMIZE_LEAP;
		double pos_y = y * SECTOR_SIZE + j * SECTOR_GEN_OPTIMIZE_LEAP;
		double pos_z = z * SECTOR_SIZE + k * SECTOR_GEN_OPTIMIZE_LEAP;

		gradient_values[i][j][k] = generate_landscape(pos_x, pos_y, pos_z);
	}

	for(uint32_t i = 0; i < size - 1; i++)
	for(uint32_t j = 0; j < size - 1; j++)
	for(uint32_t k = 0; k < size - 1; k++)
	{
		double aaa = gradient_values[i][j][k];
		double baa = gradient_values[i + 1][j][k];
		double aba = gradient_values[i][j + 1][k];
		double bba = gradient_values[i + 1][j + 1][k];
		double aab = gradient_values[i][j][k + 1];
		double bab = gradient_values[i + 1][j][k + 1];
		double abb = gradient_values[i][j + 1][k + 1];
		double bbb = gradient_values[i + 1][j + 1][k + 1];

		for(uint32_t x = 0; x < SECTOR_GEN_OPTIMIZE_LEAP; x++)
		for(uint32_t y = 0; y < SECTOR_GEN_OPTIMIZE_LEAP; y++)
		for(uint32_t z = 0; z < SECTOR_GEN_OPTIMIZE_LEAP; z++)
		{
			uint32_t ix = i * SECTOR_GEN_OPTIMIZE_LEAP + x;
			uint32_t iy = j * SECTOR_GEN_OPTIMIZE_LEAP + y;
			uint32_t iz = k * SECTOR_GEN_OPTIMIZE_LEAP + z;

			double dx = (double) x / SECTOR_GEN_OPTIMIZE_LEAP;
			double dy = (double) y / SECTOR_GEN_OPTIMIZE_LEAP;
			double dz = (double) z / SECTOR_GEN_OPTIMIZE_LEAP;

			voxels[ix][iy][iz] = math::interp_linear_3d(aaa, baa, aba, bba, aab, bab, abb, bbb, dx, dy, dz) < 0;
		}
	}
	
	for(size_t i = 0; i < size; i++)
	{
		for(size_t j = 0; j < size; j++)
			delete[] gradient_values[i][j];
		delete[] gradient_values[i];
	}
	delete[] gradient_values;
#else
	for(uint32_t i = 0; i < SECTOR_SIZE; i++)
	for(uint32_t j = 0; j < SECTOR_SIZE; j++)
	for(uint32_t k = 0; k < SECTOR_SIZE; k++)
	{
		double pos_x = x * SECTOR_SIZE + i;
		double pos_y = y * SECTOR_SIZE + j;
		double pos_z = z * SECTOR_SIZE + k;

		voxels[i][j][k] = generate_landscape(pos_x, pos_y, pos_z) < 0;
	}
#endif

	state = SECTOR_STATE_GENERATED;
}

void sector::get_pos(int64_t* pos_x, int64_t* pos_y, int64_t* pos_z)
{
	*pos_x = x;
	*pos_y = y;
	*pos_z = z;
}

uint8_t sector::get_state() const
{
	return state;
}

void sector::load_mesh()
{
	uint16_t bound = SECTOR_SIZE - 1;
	
	char*** facing = new char**[SECTOR_SIZE];
	for(uint16_t i = 0; i < SECTOR_SIZE; i++)
	{
		facing[i] = new char*[SECTOR_SIZE];
		for(uint16_t j = 0; j < SECTOR_SIZE; j++)
			facing[i][j] = new char[SECTOR_SIZE];
	}
	
	for(uint32_t i = 0; i < SECTOR_SIZE; i++)
	for(uint32_t j = 0; j < SECTOR_SIZE; j++)
	for(uint32_t k = 0; k < SECTOR_SIZE; k++)
	{
		facing[i][j][k] = 0;
		
		if(voxels[i][j][k] != 0)
		{
			facing[i][j][k] |= ((char) (i == 0 || voxels[i - 1][j][k] == 0)) << FACE_LEFT;
			facing[i][j][k] |= ((char) (i == bound || voxels[i + 1][j][k] == 0)) << FACE_RIGHT;
			facing[i][j][k] |= ((char) (j == 0 || voxels[i][j - 1][k] == 0)) << FACE_BOTTOM;
			facing[i][j][k] |= ((char) (j == bound || voxels[i][j + 1][k] == 0)) << FACE_TOP;
			facing[i][j][k] |= ((char) (k == 0 || voxels[i][j][k - 1] == 0)) << FACE_FRONT;
			facing[i][j][k] |= ((char) (k == bound || voxels[i][j][k + 1] == 0)) << FACE_BACK;
		}
	}
	
	uint32_t index_count = 0;
	
	bool should_loop = true;
	while(should_loop)
	{
		should_loop = false;
		
		for(float i = 0; i < SECTOR_SIZE; i++)
		for(float j = 0; j < SECTOR_SIZE; j++)
		for(float k = 0; k < SECTOR_SIZE; k++)
		{
			bool left_facing = is_facing(facing, i, j, k, FACE_LEFT);
			bool right_facing = is_facing(facing, i, j, k, FACE_RIGHT);
			bool bottom_facing = is_facing(facing, i, j, k, FACE_BOTTOM);
			bool top_facing = is_facing(facing, i, j, k, FACE_TOP);
			bool front_facing = is_facing(facing, i, j, k, FACE_FRONT);
			bool back_facing = is_facing(facing, i, j, k, FACE_BACK);
			
			if(left_facing)
			{
				should_loop = true;
				
				float start_y = j;
				float start_z = k;
				
				float end_y = start_y + 1;
				float end_z = start_z + 1;
				
				for(uint32_t a = end_y; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, i, a, k, FACE_LEFT)) break;
					end_y++;
				}
				
				for(uint32_t b = end_z; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = j; a < end_y; a++)
					{
						if(!is_facing(facing, i, a, b, FACE_LEFT))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_z++;
				}
				
				m->add_vertex({i, start_y, start_z,   1  , 0.5, 0.5,   FACE_LEFT, 0});
				m->add_vertex({i, end_y  , start_z,   1  , 0.5, 0.5,   FACE_LEFT, 0});
				m->add_vertex({i, end_y  , end_z  ,   1  , 0.5, 0.5,   FACE_LEFT, 1});
				m->add_vertex({i, start_y, end_z  ,   1  , 0.5, 0.5,   FACE_LEFT, 1});
				
				m->add_indices({0 + index_count, 2 + index_count, 1 + index_count, 0 + index_count, 3 + index_count, 2 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_y; a < end_y; a++)
				for(uint32_t b = start_z; b < end_z; b++)
				{
					facing[(uint32_t) i][a][b] &= ~(1 << FACE_LEFT);
				}
			}
			
			if(right_facing)
			{
				should_loop = true;
				
				float start_y = j;
				float start_z = k;
				
				float end_y = start_y + 1;
				float end_z = start_z + 1;
				
				for(uint32_t a = end_y; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, i, a, k, FACE_RIGHT)) break;
					end_y++;
				}
				
				for(uint32_t b = end_z; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = j; a < end_y; a++)
					{
						if(!is_facing(facing, i, a, b, FACE_RIGHT))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_z++;
				}
				
				m->add_vertex({i + 1, start_y, start_z,   0.5, 1  , 0.5,   FACE_RIGHT, 0});
				m->add_vertex({i + 1, end_y  , start_z,   0.5, 1  , 0.5,   FACE_RIGHT, 0});
				m->add_vertex({i + 1, end_y  , end_z  ,   0.5, 1  , 0.5,   FACE_RIGHT, 1});
				m->add_vertex({i + 1, start_y, end_z  ,   0.5, 1  , 0.5,   FACE_RIGHT, 1});
				
				m->add_indices({0 + index_count, 1 + index_count, 2 + index_count, 0 + index_count, 2 + index_count, 3 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_y; a < end_y; a++)
				for(uint32_t b = start_z; b < end_z; b++)
				{
					facing[(uint32_t) i][a][b] &= ~(1 << FACE_RIGHT);
				}
			}
			
			if(bottom_facing)
			{
				should_loop = true;
				
				float start_x = i;
				float start_z = k;
				
				float end_x = start_x + 1;
				float end_z = start_z + 1;
				
				for(uint32_t a = end_x; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, a, j, k, FACE_BOTTOM)) break;
					end_x++;
				}
				
				for(uint32_t b = end_z; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = i; a < end_x; a++)
					{
						if(!is_facing(facing, a, j, b, FACE_BOTTOM))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_z++;
				}
				
				m->add_vertex({start_x, j, start_z,   0.5, 0.5, 1  ,   FACE_BOTTOM, 0});
				m->add_vertex({end_x  , j, start_z,   0.5, 0.5, 1  ,   FACE_BOTTOM, 0});
				m->add_vertex({end_x  , j, end_z  ,   0.5, 0.5, 1  ,   FACE_BOTTOM, 1});
				m->add_vertex({start_x, j, end_z  ,   0.5, 0.5, 1  ,   FACE_BOTTOM, 1});
				
				m->add_indices({0 + index_count, 1 + index_count, 2 + index_count, 0 + index_count, 2 + index_count, 3 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_x; a < end_x; a++)
				for(uint32_t b = start_z; b < end_z; b++)
				{
					facing[a][(uint32_t) j][b] &= ~(1 << FACE_BOTTOM);
				}
			}
			
			if(top_facing)
			{
				should_loop = true;
				
				float start_x = i;
				float start_z = k;
				
				float end_x = start_x + 1;
				float end_z = start_z + 1;
				
				for(uint32_t a = end_x; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, a, j, k, FACE_TOP)) break;
					end_x++;
				}
				
				for(uint32_t b = end_z; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = i; a < end_x; a++)
					{
						if(!is_facing(facing, a, j, b, FACE_TOP))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_z++;
				}
				
				m->add_vertex({start_x, j + 1, start_z,   1  , 1  , 0.5,   FACE_TOP, 0});
				m->add_vertex({end_x  , j + 1, start_z,   1  , 1  , 0.5,   FACE_TOP, 0});
				m->add_vertex({end_x  , j + 1, end_z  ,   1  , 1  , 0.5,   FACE_TOP, 1});
				m->add_vertex({start_x, j + 1, end_z  ,   1  , 1  , 0.5,   FACE_TOP, 1});
				
				m->add_indices({0 + index_count, 2 + index_count, 1 + index_count, 0 + index_count, 3 + index_count, 2 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_x; a < end_x; a++)
				for(uint32_t b = start_z; b < end_z; b++)
				{
					facing[a][(uint32_t) j][b] &= ~(1 << FACE_TOP);
				}
			}
			
			if(front_facing)
			{
				should_loop = true;
				
				float start_x = i;
				float start_y = j;
				
				float end_x = start_x + 1;
				float end_y = start_y + 1;
				
				for(uint32_t a = end_x; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, a, j, k, FACE_FRONT)) break;
					end_x++;
				}
				
				for(uint32_t b = end_y; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = i; a < end_x; a++)
					{
						if(!is_facing(facing, a, b, k, FACE_FRONT))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_y++;
				}
				
				m->add_vertex({start_x, start_y, k,   1  , 0.5, 1  ,   FACE_FRONT, 0});
				m->add_vertex({end_x  , start_y, k,   1  , 0.5, 1  ,   FACE_FRONT, 0});
				m->add_vertex({end_x  , end_y  , k,   1  , 0.5, 1  ,   FACE_FRONT, 1});
				m->add_vertex({start_x, end_y  , k,   1  , 0.5, 1  ,   FACE_FRONT, 1});
				
				m->add_indices({0 + index_count, 2 + index_count, 1 + index_count, 0 + index_count, 3 + index_count, 2 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_x; a < end_x; a++)
				for(uint32_t b = start_y; b < end_y; b++)
				{
					facing[a][b][(uint32_t) k] &= ~(1 << FACE_FRONT);
				}
			}
			
			if(back_facing)
			{
				should_loop = true;
				
				float start_x = i;
				float start_y = j;
				
				float end_x = start_x + 1;
				float end_y = start_y + 1;
				
				for(uint32_t a = end_x; a < SECTOR_SIZE; a++)
				{
					if(!is_facing(facing, a, j, k, FACE_BACK)) break;
					end_x++;
				}
				
				for(uint32_t b = end_y; b < SECTOR_SIZE; b++)
				{
					bool should_break = false;
					for(uint32_t a = i; a < end_x; a++)
					{
						if(!is_facing(facing, a, b, k, FACE_BACK))
						{
							should_break = true;
							break;
						}
					}
					if(should_break) break;
					end_y++;
				}
				
				m->add_vertex({start_x, start_y, k + 1,   0.5, 1  , 1  ,   FACE_BACK, 0});
				m->add_vertex({end_x  , start_y, k + 1,   0.5, 1  , 1  ,   FACE_BACK, 0});
				m->add_vertex({end_x  , end_y  , k + 1,   0.5, 1  , 1  ,   FACE_BACK, 1});
				m->add_vertex({start_x, end_y  , k + 1,   0.5, 1  , 1  ,   FACE_BACK, 1});
				
				m->add_indices({0 + index_count, 1 + index_count, 2 + index_count, 0 + index_count, 2 + index_count, 3 + index_count});
				index_count += 4;
				
				for(uint32_t a = start_x; a < end_x; a++)
				for(uint32_t b = start_y; b < end_y; b++)
				{
					facing[a][b][(uint32_t) k] &= ~(1 << FACE_BACK);
				}
			}
		}
	}
	
	//std::cout << "Vertex count: " << index_count << std::endl;
	
	for(uint16_t i = 0; i < SECTOR_SIZE; i++)
	{
		for(uint16_t j = 0; j < SECTOR_SIZE; j++)
			delete[] facing[i][j];
		delete[] facing[i];
	}
	delete[] facing;

	state = SECTOR_STATE_MESH_LOADED;
}

void sector::set(uint16_t x, uint16_t y, uint16_t z, uint32_t value, bool reload)
{
	if(x >= SECTOR_SIZE || y >= SECTOR_SIZE || z >= SECTOR_SIZE) return;
	
	voxels[x][y][z] = value;
	if(reload)
	{
		vkDeviceWaitIdle(get_device());
		load_mesh();
		build();
	}
	else
	{
		state = SECTOR_STATE_GENERATED;
	}
}