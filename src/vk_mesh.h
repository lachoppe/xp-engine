#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>


struct VertexInputDescription
{
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex
{
	glm::vec3 position;
 	glm::vec<2, uint8_t> octNormal;
 	glm::vec<3, uint8_t> color;
	glm::vec2 uv;

	static VertexInputDescription GetVertexDescription();

	void PackNormal(glm::vec3 n);
	void PackColor(glm::vec3 c);
};

struct RenderBounds
{
	glm::vec3 origin;
	float radius;
	glm::vec3 extents;
	bool isValid;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	AllocatedBuffer vertexBuffer{ nullptr, nullptr };
	AllocatedBuffer indexBuffer{ nullptr, nullptr };

	RenderBounds bounds;

	bool LoadFromAsset(const char* filename);
	bool LoadFromObj(const char* filename);

	// Deprecated
	glm::vec3 objPosMin{ 0.0f };
	glm::vec3 objPosMax{ 0.0f };
	glm::vec3 GetObjectCenter() const;
};