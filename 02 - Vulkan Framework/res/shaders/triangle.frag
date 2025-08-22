#version 450
layout(location = 0) out vec4 color;

layout(location = 0) in vec3 f_color;
layout(location = 1) in vec2 f_tex;

layout(binding = 1) uniform sampler2D tex;

void main()
{
    color = texture(tex, f_tex) * vec4(f_color, 1);
}