#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera 
{
    mat4 mvp;
} camera;

layout(set = 2, binding = 0) uniform Material 
{
    vec4 ambient;
    vec4 diffuse;
    vec4 emission;
} material;

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;

layout(location = 0) out vec3 color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = camera.mvp * vec4(positions, 1.0);
    color = material.ambient.rgb + material.diffuse.rgb * max(0.2, dot(normalize(vec3(0.5,1,0)), normals)) + material.emission.rgb;
}
