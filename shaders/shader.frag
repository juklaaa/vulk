#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;
layout(location = 5) in vec3 fragCameraPosition;
layout(location = 6) in vec3 fragPositon;
layout(location = 7) in vec3 fragLight;
layout(location = 8) in vec3 fragLightCamPosition;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSmpler;
layout(binding = 3) uniform sampler2D depthSampler;

layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;

layout(location = 0) out vec4 outColor;

vec3 directionToLight = normalize(fragLight);
vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

void main() 
{
    float lightDepth = texture(depthSampler, fragLightCamPosition.xy).r;
    // Exp -> linear conversion?
    float lit = step(fragLightCamPosition.z, lightDepth);

    vec3 normalMap = texture(normalSmpler, fragTexCoord).rgb;
    normalMap = normalMap * 2.0f - vec3(1.0f);
    vec3 normalWorld = normalize(fragTBN * normalMap);

    float lightIntensity = max(lit * dot(normalWorld, directionToLight), 0.1);

    vec3 viewDir = normalize(fragCameraPosition - fragPositon.xyz);
    vec3 reflectDir = reflect(-directionToLight, normalWorld);  
    float spec = lit * pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = spec * lightColor;

    if (pushConstants.isPPLightingEnabled == 1)
    {
        outColor = (texture(texSampler, fragTexCoord) * lightIntensity) + vec4(specular, 1.0f);    
    }
    else
    {
        outColor = vec4(lightIntensity*fragColor, 1.0f) ;        
    }
}