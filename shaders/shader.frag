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

   

    vec3 normalMap = texture(normalSmpler, fragTexCoord).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    vec3 normalWorld = normalize(fragTBN * normalMap);

    float lightIntensity = max(dot(normalWorld, DIRECTION_TO_LIGHT), 0.1);

    if (pushConstants.isPPLightingEnabled == 1)
    {
        //outColor = texture(texSampler, fragTexCoord) * lightIntensity;
        outColor = texture(normalSmpler,fragTexCoord);        
    }
    else
    {
        //outColor = texture(texSampler, fragTexCoord) * vec4(fragColor,1.0f);
        //outColor=vec4(1,1,1,0);
        outColor = vec4(lightIntensity*fragColor,0);
//        outColor = vec4(normalWorld, 0);
    }
}