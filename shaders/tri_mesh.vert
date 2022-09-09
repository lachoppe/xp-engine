#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout (set=0, binding=0) uniform CameraBuffer
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

struct ObjectData
{
	mat4 model;
};

// std140 layout description makes arrays match how they work in C++, enforcing layout and alignment
layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

void main()
{
	mat4 model = objectBuffer.objects[gl_BaseInstance].model;
	mat4 xform = (cameraData.viewProj * model);
	gl_Position = xform * vec4(vPosition, 1.0f);
	outColor = vColor;
}