#version 450
    
const int NUM_BONES = 32;
layout(binding = 0) uniform UniformBufferObject 
{   
    mat4 model;
	mat4 view;
	mat4 proj;
	mat4 depthMVP;
	vec4 light;
	vec3 modelColor;
	float modelLightReflection;
	float textured;
    
    vec3 initialBonePos[NUM_BONES];
    vec4 initialBoneRot[NUM_BONES];
    vec3 initialBoneScale[NUM_BONES];
    
    vec3 bonePos[NUM_BONES];
    vec4 boneRot[NUM_BONES];
    vec3 boneScale[NUM_BONES];
} ubo;
    
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in uvec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;
    
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;
layout(location = 5) out vec3 fragViewDir;
layout(location = 6) out vec3 fragLight;
layout(location = 7) out vec4 fragLightCamPosition;
    
const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );


void main() 
{   
    fragLight = ubo.light.xyz;
    fragTexCoord = inTexCoord;
    fragColor=ubo.modelColor;
    

    uvec4 boneIndices= inBoneIndices;

    vec3 radiusesToBone[4];
    for (int i = 0; i < 4; ++i)
        radiusesToBone[i] = inPosition - ubo.initialBonePos[boneIndices[i]];

    vec3 posFromBones = vec3(0.0f);
    for (int i = 0; i < 4; ++i)
        posFromBones += inBoneWeights[i] * (ubo.bonePos[boneIndices[i]] + radiusesToBone[i]);
    
     //fragColor = vec3( inBoneIndices.x / 32.0, inBoneIndices.y / 32.0, inBoneIndices.z / 32.0);
    

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(posFromBones, 1.0f);
    fragLightCamPosition = biasMat * ubo.depthMVP * vec4(inPosition, 1.0f);

    vec3 Positon = (ubo.model * vec4(inPosition, 1.0f)).xyz;
    vec3 CameraPosition = inverse(ubo.view)[3].xyz;
    fragViewDir = normalize(CameraPosition - Positon.xyz);    

    vec3 T = normalize(ubo.model*vec4(inTangent, 0.0f)).xyz;
    vec3 N = normalize(ubo.model*vec4(inNormal, 0.0f)).xyz;
    vec3 B = normalize(cross(N, T));
    fragTBN = mat3(T, B, N);
}   