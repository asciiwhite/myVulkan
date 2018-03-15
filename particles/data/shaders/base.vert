#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera
{
    mat4 mvp;
} camera;


layout(location = 0) in vec2 positions;
layout(location = 1) in vec4 age;

layout(location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main()
{
    outColor = vec4(age.x, 0, 0, 1.0);

    gl_PointSize = 4.0;
    gl_Position = camera.mvp * vec4(positions, 0.0, 1.0);
}
