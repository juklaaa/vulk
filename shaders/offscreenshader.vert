#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 model;
    mat4 view;
    mat4 proj;
    mat4 depthVP;
    vec3 light;
} ubo;
 
void main()
{
	gl_Position =  ubo.depthVP * ubo.model* vec4(inPos, 1.0);
}