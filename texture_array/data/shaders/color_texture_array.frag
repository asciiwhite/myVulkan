#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texCoord;

layout(set = 2, binding = 0) uniform sampler texSampler;
layout(set = 2, binding = 1) uniform texture2D textures[19];

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PER_OBJECT
{
	int imgIdx;
}pc;

void main()
{
    outColor = vec4(color, 1) * texture(sampler2D(textures[pc.imgIdx], texSampler), texCoord);
}
