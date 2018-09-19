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
    float opacity = 0.0;
    if (age.x > 0.0)
        opacity = 1.0 - age.x;

    float hitCount = age.y;

    outColor = vec4(1, hitCount, hitCount - 1, opacity);

    gl_PointSize = 4.0;
    gl_Position = camera.mvp * vec4(positions, 0.0, 1.0);
}
