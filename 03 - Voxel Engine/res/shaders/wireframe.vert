#version 450
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 f_color;

layout(push_constant, std430) uniform push_data
{
	mat4 transform;
	float shader_id;
} pd;

layout(binding = 0) uniform model
{
    mat4 projection;
    mat4 view;
} ubo;

void main()
{
    f_color = color;
    gl_Position = ubo.projection * ubo.view * pd.transform * vec4(pos, 1);
}