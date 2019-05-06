#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sTexture;

layout(set = 0, binding = 1) uniform Parameter
{
    float preFilterThreshold;
    float intensity;
} parameter;

layout (location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

vec3 prefilter(vec3 color)
{
//	float brightness = dot(color, vec3(0.299, 0.587, 0.114));
//  float contribution = smoothstep(parameter.threshold, 1.0, brightness);
	float brightness = max(color.r, max(color.g, color.b));
	float contribution = max(0.0, brightness - parameter.preFilterThreshold);
	contribution /= max(brightness, 0.00001);
	return color * contribution;
}

vec4 boxFilter(float texelOffset)
{
    vec4 offset = vec4(1.0 / textureSize(sTexture, 0).xyxy) * vec2(-texelOffset, texelOffset).xxyy;
    vec4 outColor = texture(sTexture, texCoords + offset.xy) + texture(sTexture, texCoords + offset.zy) +
                    texture(sTexture, texCoords + offset.xw) + texture(sTexture, texCoords + offset.zw);
    return outColor * 0.25;
}

void main()
{
    outColor = vec4(prefilter(boxFilter(1.0).rgb), 1.0);
}
