#version 450
layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 f_pos;

layout(binding = 0) uniform model
{
	mat4 transform;
    mat4 projection;
    mat4 view;
} ubo;

void main()
{
    f_pos = pos;
    gl_Position = ubo.projection * ubo.view * ubo.transform * vec4(pos, 1);
}