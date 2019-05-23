#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sMainCocTexture;

layout (location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform Parameter
{
    float nearPlane;
    float farPlane;
    float focusDistance;
    float focusRange;
    float bokehRadius;
} parameter;

const int kernelSampleCount = 22;
const vec2 kernel[kernelSampleCount] = {
	vec2(0, 0),
	vec2(0.53333336, 0),
	vec2(0.3325279, 0.4169768),
	vec2(-0.11867785, 0.5199616),
	vec2(-0.48051673, 0.2314047),
	vec2(-0.48051673, -0.23140468),
	vec2(-0.11867763, -0.51996166),
	vec2(0.33252785, -0.4169769),
	vec2(1, 0),
	vec2(0.90096885, 0.43388376),
	vec2(0.6234898, 0.7818315),
	vec2(0.22252098, 0.9749279),
	vec2(-0.22252095, 0.9749279),
	vec2(-0.62349, 0.7818314),
	vec2(-0.90096885, 0.43388382),
	vec2(-1, 0),
	vec2(-0.90096885, -0.43388376),
	vec2(-0.6234896, -0.7818316),
	vec2(-0.22252055, -0.974928),
	vec2(0.2225215, -0.9749278),
	vec2(0.6234897, -0.7818316),
	vec2(0.90096885, -0.43388376),
};

float weight(float coc, float radius)
{
	return clamp((coc - radius + 2) / 2, 0.0, 1.0);
}

void main()
{
    vec3 bgColor = vec3(0);
    vec3 fgColor = vec3(0);
	float bgWeight = 0;
    float fgWeight = 0;
    const vec2 texelSize = vec2(1.0 / textureSize(sMainCocTexture, 0).xy);

	for (int k = 0; k < kernelSampleCount; k++) {
		vec2 o = kernel[k] * parameter.bokehRadius;
        float radius = length(o);
        o *= texelSize;
        vec4 s = texture(sMainCocTexture, texCoords + o);

        float bgw = weight(max(0, s.a), radius);
    	bgColor += s.rgb * bgw;
	    bgWeight += bgw;

        float fgw = weight(-s.a, radius);
    	fgColor += s.rgb * fgw;
	    fgWeight += fgw;
    }
    bgColor /= bgWeight + ((bgWeight == 0) ? 1 : 0);
    fgColor /= fgWeight + ((fgWeight == 0) ? 1 : 0);
    float bgfg = min(1, fgWeight * 3.14159265359 / kernelSampleCount);
	outColor = vec4(mix(bgColor, fgColor, bgfg), bgfg);
}
