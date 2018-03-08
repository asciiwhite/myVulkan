#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera
{
    mat4 mvp;
} camera;

layout(location = 0) in vec3 positions;
layout(location = 1) in float groundHeight;

layout(location = 0) out vec3 color;

out gl_PerVertex {
    vec4 gl_Position;
};

float normalizeToColor(float value, float scale)
{
    return fract(abs(value * scale));
}

void main()
{
    const float elementSize = 2.0f;
    const float elementDistance = 0.1f;
    const int numElementsPerDimension = 80;
    const float groundSize = numElementsPerDimension * (elementSize + elementDistance);
    float row = gl_InstanceIndex % numElementsPerDimension;
    float column = gl_InstanceIndex / numElementsPerDimension;

    vec3 instancePos;
    instancePos.xz = -0.5f * groundSize + positions.xz + (elementSize + elementDistance) * vec2(row, column);
    instancePos.y = positions.y * groundHeight;

    color.x = normalizeToColor(groundHeight, 0.03);
    color.y = normalizeToColor(groundHeight, 0.04);
    color.z = normalizeToColor(groundHeight, 0.06);

    gl_Position = camera.mvp * vec4(instancePos, 1.0);
}
