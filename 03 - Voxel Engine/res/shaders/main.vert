#version 450
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec3 f_color;
layout(location = 2) out vec2 f_tex;
layout(location = 3) out uvec4 f_selection;
layout(location = 4) out float shader_id;
layout(location = 5) out vec3 f_tpos;

layout(push_constant, std430) uniform push_data
{
	mat4 transform;
	float shader_id;
} pd;

layout(binding = 0) uniform model
{
    mat4 projection;
    mat4 view;
	uvec4 selection;
} ubo;

void main()
{
	vec4 t_pos = pd.transform * vec4(pos, 1);

	f_pos = pos;
    f_color = color;
	f_tex = tex;
	f_selection = ubo.selection;
	shader_id = pd.shader_id;
	f_tpos = t_pos.xyz;
	
    gl_Position = ubo.projection * ubo.view * t_pos;
}