#version 450

layout(location = 0) out vec3 f_color;

vec2 pos[3] = vec2[]
(
    vec2(0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 col[3] = vec3[]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
    f_color = col[gl_VertexIndex];
}