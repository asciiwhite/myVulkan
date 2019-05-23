#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sMainTexture;
layout (binding = 1) uniform sampler2D sDoFTexture;
layout (binding = 2) uniform sampler2D sCoCTexture;

layout (location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 source = texture(sMainTexture, texCoords);
	float coc = texture(sCoCTexture, texCoords).r;
	vec4 dof = texture(sDoFTexture, texCoords);

	float dofStrength = smoothstep(0.1, 1, abs(coc));
	vec3 color = mix(source.rgb, dof.rgb, dofStrength + dof.a - dofStrength * dof.a);
	outColor = vec4(color, source.a);
}
