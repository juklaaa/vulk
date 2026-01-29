#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;
layout(location = 5) in vec3 fragViewDir;
layout(location = 6) in vec3 fragLight;
layout(location = 7) in vec4 fragLightCamPosition;

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
	mat4 view;
	mat4 proj;
	mat4 depthMVP;
	vec4 light;
	vec4 modelColor;
	float modelLightReflection;
	float textured;
    float padding1;
    float padding2;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSmpler;
layout(binding = 3) uniform sampler2D depthSampler;

/*layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;*/

layout(location = 0) out vec4 outColor;

vec3 directionToLight = normalize(fragLight);
vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

float lightReflection = ubo.modelLightReflection;
float textured = ubo.textured;

void main() 
{
    
    float lightDepth = texture(depthSampler, fragLightCamPosition.xy).r;
    float lit = step(fragLightCamPosition.z, lightDepth);

    vec3 normalMap = texture(normalSmpler, fragTexCoord).rgb;
    normalMap = normalMap * 2.0f - vec3(1.0f);
    vec3 normalWorld = normalize(fragTBN * normalMap);    

    float lightIntensity = max(lit * dot(normalWorld, directionToLight), 0.1);

    if (textured > 0)
    {
        outColor = vec4(fragColor,1.0f)*(texture(texSampler, fragTexCoord) * lightIntensity);
    }
    else
    {
        outColor = vec4(fragColor,1.0f) * lightIntensity;
    }

    if (lightReflection > 0)
    {
        vec3 reflectDir = reflect(-directionToLight, normalWorld);  
        float spec = lit * pow(max(dot(fragViewDir, reflectDir), 0.0), lightReflection);
        vec3 specular = spec * lightColor;

        outColor += vec4(specular, 1.0f);   
    }
}
