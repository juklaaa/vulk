#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSmpler;

layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;

layout(location = 0) out vec4 outColor;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(5.0f, 3.0f, 1.0f));//zmienic to na zmienialne

void main() 
{
    vec3 normalMap = texture(normalSmpler, fragTexCoord).rgb;
    normalMap = normalMap * 2.0f - vec3(1.0f);
    vec3 normalWorld = normalize(fragTBN * normalMap);

    float lightIntensity = max(dot(normalWorld, DIRECTION_TO_LIGHT), 0.1);

    if (pushConstants.isPPLightingEnabled == 1)
    {
        outColor = texture(texSampler, fragTexCoord) * lightIntensity;    
    }
    else
    {
        outColor = vec4(lightIntensity*fragColor, 1.0f);
    }
}