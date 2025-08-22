#version 450
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 f_color;
layout(location = 1) out vec2 f_tex;

layout(binding = 0) uniform model
{
	mat4 transform;
} ubo;

void main()
{
    f_color = color;
    f_tex = tex;
    gl_Position = ubo.transform * vec4(pos, 0, 1);
}