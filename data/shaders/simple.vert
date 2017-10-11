#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformBufferObject 
{
    float scale;
} ubo;

layout(location = 0) in vec2 positions;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec3 colors;

layout(location = 0) out vec3 color;
layout(location = 1) out vec2 texCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(positions, 0.0, 1.0);
    color = colors;
    texCoord = texCoords * ubo.scale;
}