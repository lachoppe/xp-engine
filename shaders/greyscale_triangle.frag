#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main()
{
	float grey = dot( inColor, vec3( 0.2126, 0.7152, 0.0722 ));
	outFragColor = vec4( grey, grey, grey, 1.0f );
}