#version 450
layout(location = 0) out uvec4 color;

layout(location = 0) in vec3 f_pos;
layout(location = 1) in vec2 f_tex;
layout(location = 2) in float sector_id;

void main()
{
	int face = int (round(f_tex.x));
	int sel_x, sel_y, sel_z;

	if(face == 0 || face == 1)
		sel_x = int (round(f_pos.x)) << 16;
	else
		sel_x = int (floor(f_pos.x)) << 16;

	if(face == 2 || face == 3)
		sel_y = int (round(f_pos.y)) << 8;
	else
		sel_y = int (floor(f_pos.y)) << 8;

	if(face == 4 || face == 5)
		sel_z = int (round(f_pos.z));
	else
		sel_z = int (floor(f_pos.z));
	
	int sel = sel_x | sel_y | sel_z;
	
    color = uvec4(sel, int (round(f_tex.x)), int (round(sector_id)), 1);
}