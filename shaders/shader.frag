#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in mat3 fragTBN;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSmpler;

layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;

layout(location = 0) out vec4 outColor;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(5.0f, 3.0f, 1.0f));

void main() 
{
//    float lightIntensity = max(dot(normalize(normalWorldSpace), DIRECTION_TO_LIGHT), 0.1f);
    //outColor = texture(texSampler, fragTexCoord) * lightIntensity;

    vec3 fixedNormalMap = vec3(0.5f, 0.5f, 1.0f);
    vec3 normalMap = texture(normalSmpler, fragTexCoord).rgb;
    fixedNormalMap = fixedNormalMap * 2.0f - vec3(1.0f);
    normalMap = normalMap * 2.0f - vec3(1.0f);
    vec3 fixedNormalWorld = normalize(fragTBN * fixedNormalMap);
    vec3 normalWorld = normalize(fragTBN * normalMap);

    float lightIntensity = max(dot(normalWorld, DIRECTION_TO_LIGHT), 0.1);

    if (pushConstants.isPPLightingEnabled == 1)
    {
        //outColor = texture(texSampler, fragTexCoord) * lightIntensity;
        outColor = vec4(texture(normalSmpler,fragTexCoord).rgb, 1.0f);        
    }
    else
    {
        //outColor = texture(texSampler, fragTexCoord) * vec4(fragColor,1.0f);
        //outColor=vec4(1,1,1,0);
        outColor = vec4(lightIntensity*fragColor, 1.0f);
        //outColor = vec4(fragTexCoord, 0.0f, 1.0f);
        //outColor = texture(normalSmpler,fragTexCoord);        
        //outColor = vec4(normalWorld, 1.0f);// + vec4(0.5f, 0.5f, 0.5f, 1.0f);
        //outColor = 0.5f * vec4(fragColor, 1.0f) + vec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}