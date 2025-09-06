#version 450
layout(location = 0) out vec4 color;

layout(location = 0) in vec3 f_pos;
layout(location = 1) in vec3 f_color;
layout(location = 2) in vec2 f_tex;
layout(location = 3) flat in uvec4 f_selection;

#define EPSILON 0.0001

#define FACE_LEFT 0
#define FACE_RIGHT 1
#define FACE_BOTTOM 2
#define FACE_TOP 3
#define FACE_FRONT 4
#define FACE_BACK 5

int shift_wraparound_right(uint x, uint shift)
{
	uint r = x << shift;
	r |= (x >> (32 - shift));
	return int (r);
}

int shift_wraparound_left(uint x, uint shift)
{
	uint r = x >> shift;
	r |= (x << (32 - shift));
	return int (r);
}

vec3 random_color(int x, int y, int z)
{
	int ap = (y % 7);
	int bp = (z % 11);
	int cp = (z % 13);
	
	int a = x ^ 0x1a39d10b + (y << ap);
	int b = y ^ 0xc79ad3e9 + (z << bp);
	int c = z ^ 0x28e9a211 + (x << cp);
	
	for(int i = 0; i < 3; i++)
	{
		int ra = shift_wraparound_right(a, (b % 11));
		int rb = shift_wraparound_right(b, (c % 11));
		int rc = shift_wraparound_right(c, (a % 11));
		
		ra ^= shift_wraparound_left(c, (rb % 7));
		rb ^= shift_wraparound_left(a, (rc % 7));
		rc ^= shift_wraparound_left(b, (ra % 7));
		
		a = ra;
		b = rb;
		c = rc;
	}
	
	a &= 255;
	b &= 255;
	c &= 255;
	
	return vec3(float (a) / 255.0, float (b) / 255.0, float (c) / 255.0);
}

float interp_linear_1d(float a, float b, float t)
{
	return t * (b - a) + a;
}

float interp_linear_2d(float aa, float ba, float ab, float bb, float t, float tt)
{
	return interp_linear_1d(interp_linear_1d(aa, ba, t), interp_linear_1d(ab, bb, t), tt);
}

float interp_linear_3d(float aaa, float baa, float aba, float bba, float aab, float bab, float abb, float bbb, float t, float tt, float ttt)
{
	return interp_linear_1d(interp_linear_2d(aaa, baa, aba, bba, t, tt), interp_linear_2d(aab, bab, abb, bbb, t, tt), ttt);
}

vec3 get_smooth_random_color()
{
	int ax = int (floor(f_pos.x / 16 + EPSILON));
	int ay = int (floor(f_pos.y / 16 + EPSILON));
	int az = int (floor(f_pos.z / 16 + EPSILON));
	
	int bx = ax + 1;
	int by = ay + 1;
	int bz = az + 1;
	
	float dx = (f_pos.x - ax * 16);
	float dy = (f_pos.y - ay * 16);
	float dz = (f_pos.z - az * 16);
	
	dx = floor(dx);
	dy = floor(dy);
	dz = floor(dz);
	
	dx /= 16;
	dy /= 16;
	dz /= 16;
	
	vec3 aaa = random_color(ax, ay, az);
	vec3 baa = random_color(bx, ay, az);
	vec3 aba = random_color(ax, by, az);
	vec3 bba = random_color(bx, by, az);
	vec3 aab = random_color(ax, ay, bz);
	vec3 bab = random_color(bx, ay, bz);
	vec3 abb = random_color(ax, by, bz);
	vec3 bbb = random_color(bx, by, bz);
	
	float x = interp_linear_3d(aaa.x, baa.x, aba.x, bba.x, aab.x, bab.x, abb.x, bbb.x, dx, dy, dz);
	float y = interp_linear_3d(aaa.y, baa.y, aba.y, bba.y, aab.y, bab.y, abb.y, bbb.y, dx, dy, dz);
	float z = interp_linear_3d(aaa.z, baa.z, aba.z, bba.z, aab.z, bab.z, abb.z, bbb.z, dx, dy, dz);
	
	return (vec3(x, y, z) - vec3(0.5, 0.5, 0.5)) / 4;
}

vec3 get_random_color()
{
	int px = int (floor(f_pos.x * 4 + EPSILON));
	int py = int (floor(f_pos.y * 4 + EPSILON));
	int pz = int (floor(f_pos.z * 4 + EPSILON));
	
	vec3 r = (random_color(px, py, pz) - vec3(0.5, 0.5, 0.5)) / 8;
	return r;
}

void main()
{
	int face = int (round(f_tex.x));
	vec3 normal;
	
	switch(face)
	{
		case FACE_LEFT:
			normal = vec3(-1, 0, 0);
			break;
		case FACE_RIGHT:
			normal = vec3(1, 0, 0);
			break;
		case FACE_BOTTOM:
			normal = vec3(0, -1, 0);
			break;
		case FACE_TOP:
			normal = vec3(0, 1, 0);
			break;
		case FACE_FRONT:
			normal = vec3(0, 0, -1);
			break;
		case FACE_BACK:
			normal = vec3(0, 0, 1);
			break;
	}
	
	float light = (dot(normalize(vec3(1, 2, 1)), normal) + 1) / 2;
	light = max(light, 0.1);
	
	vec3 r = get_random_color();
	vec3 sr = get_smooth_random_color();
	
	int sel_x = int (round(f_selection.x)) >> 16;
	int sel_y = (int (round(f_selection.x)) >> 8) & 255;
	int sel_z = int (round(f_selection.x)) & 255;

	float s_x = float (sel_x);
	float s_y = float (sel_y);
	float s_z = float (sel_z);
	
    color = vec4((vec3(0.27, 0.9, 0.2) + r + sr) * light, 1);
	
	/*
	float p_x = f_pos.x + EPSILON - floor(f_pos.x + EPSILON);
	float p_y = f_pos.y + EPSILON - floor(f_pos.y + EPSILON);
	float p_z = f_pos.z + EPSILON - floor(f_pos.z + EPSILON);

	color = vec4(p_x, p_y, p_z, 1);
	*/

	if(f_pos.x >= s_x - EPSILON && f_pos.y >= s_y - EPSILON && f_pos.z >= s_z - EPSILON && f_pos.x <= s_x + 1 + EPSILON && f_pos.y < s_y + 1 + EPSILON && f_pos.z < s_z + 1 + EPSILON)
	{
		color += vec4(0.15, 0.15, 0.15, 0);
	}

	/*
	if(f_selection.w != 0)
	{
		switch(face)
		{
			case FACE_LEFT:
			case FACE_RIGHT:
				
			break;
			case FACE_BOTTOM:
			case FACE_TOP:
				if(f_pos.x >= s_x && f_pos.y >= s_y && f_pos.z >= s_z && f_pos.x < s_x + 1 && f_pos.y <= s_y + 1 && f_pos.z < s_z + 1)
				{
					color += vec4(0.15, 0.15, 0.15, 0);
				}
			break;
			case FACE_FRONT:
			case FACE_BACK:
				if(f_pos.x >= s_x && f_pos.y >= s_y && f_pos.z >= s_z && f_pos.x < s_x + 1 && f_pos.y < s_y + 1 && f_pos.z <= s_z + 1)
				{
					color += vec4(0.15, 0.15, 0.15, 0);
				}
			break;
		}
	}
	*/
}