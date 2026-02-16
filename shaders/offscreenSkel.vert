#version 450
    
const int NUM_BONES = 64;
layout(binding = 0) uniform UniformBufferObject 
{   
    mat4 MVP;
    
    vec4 initialBonePos[NUM_BONES];
    vec4 initialBoneRot[NUM_BONES];
    vec4 initialBoneScale[NUM_BONES];
    
    vec4 bonePos[NUM_BONES];
    vec4 boneRot[NUM_BONES];
    vec4 boneScale[NUM_BONES];
} ubo;
    
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in uvec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;
    

struct Quat
{
    float x;
    float y;
    float z;
    float w;
};

Quat normalizeQuat(Quat q)
{
    float len = sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    return Quat(q.x/len, q.y/len, q.z/len, q.w/len);
}


Quat multiplicteQuat(Quat q1, Quat q2)
{
	return 
	Quat(
		q1.w * q2.x + q1.x * q2.w - q1.y * q2.z + q1.z * q2.y,
		q1.w * q2.y + q1.x * q2.z + q1.y * q2.w - q1.z * q2.x,
		q1.w * q2.z - q1.x * q2.y + q1.y * q2.x + q1.z * q2.w,
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
	);
}
	
Quat inversQuat(Quat q) 
{
	return Quat(-q.x, -q.y, -q.z, q.w);
}

vec3 QuatToVec(Quat q)
{
    return vec3(q.x, q.y, q.z);
}

Quat VecToQuat(vec3 v)
{
    return Quat(v.x,v.y,v.z,0.0f);
}

vec3 rotateVectorByQuat(vec3 v, Quat q)
{
     return QuatToVec(multiplicteQuat(inversQuat(q), multiplicteQuat(VecToQuat(v),q)));

}

void main() 
{   
     vec3 finalBoneTransform = vec3(0.0f);
    for(int i=0;i<4;i++)
    {
        uint ind=inBoneIndices[i];
        Quat initBoneRot = normalizeQuat(Quat(ubo.initialBoneRot[ind].x,ubo.initialBoneRot[ind].y,ubo.initialBoneRot[ind].z,ubo.initialBoneRot[ind].w));
        Quat boneRot =normalizeQuat(Quat(ubo.boneRot[ind].x,ubo.boneRot[ind].y,ubo.boneRot[ind].z,ubo.boneRot[ind].w));

        vec3 localPos = inPosition - ubo.initialBonePos[ind].xyz;
        
        vec3 rot = rotateVectorByQuat(localPos,inversQuat(initBoneRot));        
        rot = rotateVectorByQuat(rot,boneRot);      

        vec3 transform = rot + ubo.bonePos[ind].xyz;

        finalBoneTransform += inBoneWeights[i] * transform;
    }

    gl_Position = ubo.MVP * vec4(finalBoneTransform, 1.0f);
    
}   