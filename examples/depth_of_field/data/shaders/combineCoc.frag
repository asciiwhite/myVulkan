#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D sMainTexture;
layout (binding = 1) uniform sampler2D sCoCTexture;

layout (location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 offset = vec4(1.0 / textureSize(sCoCTexture, 0).xyxy) * vec2(-0.5, 0.5).xxyy;
    vec4 coc;
    coc.r = texture(sCoCTexture, texCoords + offset.xy).r;
    coc.g = texture(sCoCTexture, texCoords + offset.zy).r;
    coc.b = texture(sCoCTexture, texCoords + offset.xw).r;
    coc.a = texture(sCoCTexture, texCoords + offset.zw).r;

    float cocMin = min(min(min(coc.r, coc.g), coc.b), coc.a);
    float cocMax = max(max(max(coc.r, coc.g), coc.b), coc.a);
    float newCoc = cocMax >= -cocMin ? cocMax : cocMin;
    outColor = vec4(texture(sMainTexture, texCoords).rgb, newCoc);
}
