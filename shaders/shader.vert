#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

const vec3 DIRECTION_TO_LIGHT=normalize(vec3(5.0,3.0,1.0));

void main() {

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    vec3 normalWorldSpace= normalize(mat3(ubo.model)*normal);
    float lightIntensty = max(dot(normalWorldSpace,DIRECTION_TO_LIGHT),0.1);

    

    fragColor = lightIntensty*inColor;
    
    //fragTexCoord = inTexCoord;
    //fragColor=normalWorldSpace;
}