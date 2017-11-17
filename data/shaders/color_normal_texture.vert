#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec3 colors;
layout(location = 3) in vec2 texCoords;

layout(location = 0) out vec3 color;
layout(location = 1) out vec2 texCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = ubo.mvp * vec4(positions, 1.0);
    color = colors * dot(vec3(0,1,0), normals);
    texCoord = texCoords;
}