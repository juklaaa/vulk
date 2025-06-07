#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normalWorldSpace;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(5.0f, 3.0f, 1.0f));

void main() {

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0f);
    fragTexCoord = inTexCoord;
    normalWorldSpace = normalize(ubo.model * vec4(inNormal, 0.0f)).xyz;
    if (pushConstants.isPPLightingEnabled == 1)
    {
        fragColor = vec3(1.0f, 1.0f, 1.0f);
    }
    else
    {
        fragColor = vec3(1.0f, 1.0f, 1.0f) * max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0.1f);
    }    
}