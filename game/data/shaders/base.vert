#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera 
{
    mat4 mvp;
} camera;

layout(location = 0) in vec3 positions;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    const float elementSize = 2.0f;
    const float elementDistance = 0.1f;
    const int numElementsPerSize = 10;
    const float groundSize = numElementsPerSize * (elementSize + elementDistance);

    vec3 instancePos = positions;
    instancePos.xz = -0.5f * groundSize + positions.xz + (elementSize + elementDistance) * vec2(gl_InstanceIndex / numElementsPerSize, gl_InstanceIndex % numElementsPerSize);
    instancePos.y = sin(float(gl_InstanceIndex) / 100.f) * 10.f;

    gl_Position = camera.mvp * vec4(instancePos, 1.0);
}
