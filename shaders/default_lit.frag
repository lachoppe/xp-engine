#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform SceneData {
	vec4 fogColor;			// w: exponent
	vec4 fogDistances;		// x: min, y: max, zw: unused
	vec4 ambientColor;
	vec4 sunlightDir;		// w: intensity
	vec4 sunlightColor;
} sceneData;

void main()
{
	vec3 lit = inColor;
	
	lit += sceneData.ambientColor.xyz;

	outFragColor = vec4( lit, 1.0f );
}