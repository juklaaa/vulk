#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
    mat4 MVP;
} ubo;
 
void main()
{
	gl_Position =  ubo.MVP * vec4(inPos, 1.0);
}