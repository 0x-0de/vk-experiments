#version 450
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 f_color;

layout(binding = 0) uniform model
{
	mat4 transform;
    mat4 projection;
    mat4 view;
} ubo;

void main()
{
    f_color = color;
    gl_Position = ubo.projection * ubo.view * ubo.transform * vec4(pos, 1);
}