#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sTexture;
layout (binding = 1) uniform sampler2D sSource;

layout(set = 0, binding = 2) uniform Parameter
{
    float preFilterThreshold;
    float intensity;
} parameter;

layout (location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 offset = vec4(1.0 / textureSize(sTexture, 0).xyxy) * vec2(-0.5, 0.5).xxyy;
    outColor = texture(sTexture, texCoords + offset.xy) + texture(sTexture, texCoords + offset.zy) +
               texture(sTexture, texCoords + offset.xw) + texture(sTexture, texCoords + offset.zw);
    outColor *= 0.25 * parameter.intensity;

    outColor += texture(sSource, texCoords);
}
