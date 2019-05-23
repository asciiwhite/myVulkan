#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sDepthTexture;

layout (location = 0) in vec2 texCoords;

layout(set = 0, binding = 1) uniform Parameter
{
    float nearPlane;
    float farPlane;
    float focusDistance;
    float focusRange;
    float bokehRadius;
} parameter;

layout(location = 0) out vec4 outColor;

float LinearEyeDepth(float depth)
{
    depth = 2.0 * depth - 1.0;
    return 2.0 * parameter.nearPlane * parameter.farPlane / (parameter.farPlane + parameter.nearPlane - depth * (parameter.farPlane - parameter.nearPlane));
}

void main()
{
    float depth = texture(sDepthTexture, texCoords).r;
    depth = LinearEyeDepth(depth);
    float coc = (depth - parameter.focusDistance) / parameter.focusRange;
    coc = clamp(coc, -1, 1) * parameter.bokehRadius;
    outColor = vec4(coc);
}
