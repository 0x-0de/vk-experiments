#version 450
layout(location = 0) in vec3 pos;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec2 f_tex;
layout(location = 2) out float sector_id;

layout(push_constant, std430) uniform push_data
{
    mat4 transform;
    float sector_id;
} pd;

layout(binding = 0) uniform model
{
    mat4 projection;
    mat4 view;
} ubo;

void main()
{
    f_pos = pos;
	f_tex = tex;
    sector_id = pd.sector_id;
	
    gl_Position = ubo.projection * ubo.view * pd.transform * vec4(pos, 1);
}