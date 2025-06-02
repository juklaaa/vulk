#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normalWorldSpace;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(5.0f, 3.0f, 1.0f));

void main() 
{
    float lightIntensity = max(dot(normalize(normalWorldSpace), DIRECTION_TO_LIGHT), 0.1f);
    //outColor = texture(texSampler, fragTexCoord) * lightIntensity;
    outColor = vec4(1.0f) * lightIntensity;
}