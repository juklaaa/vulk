#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 light;
} ubo;

layout(push_constant) uniform constants
{
    uint isPPLightingEnabled;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;
layout(location = 5) out vec3 fragCameraPosition;
layout(location = 6) out vec4 fragPositon;
layout(location = 7) out vec3 fragLight;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(5.0f, 3.0f, 1.0f));

void main() {

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0f);

    fragLight = ubo.light;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragCameraPosition =vec3(ubo.model * vec4(inPosition, 1.0f));
    fragCameraPosition = inverse(ubo.view)[3].xyz;

    vec3 T = normalize(ubo.model*vec4(inTangent, 0.0f)).xyz;
    vec3 N = normalize(ubo.model*vec4(inNormal, 0.0f)).xyz;
    vec3 B = normalize(cross(N, T));
    fragTBN = mat3(T, B, N);

    
}