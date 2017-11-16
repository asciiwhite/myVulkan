#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 positions;
layout(location = 2) in vec3 colors;

layout(location = 0) out vec3 color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = ubo.mvp * vec4(positions, 1.0);
    color = colors;
}