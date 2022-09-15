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
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;

	static VertexInputDescription GetVertexDescription();
};

struct Mesh
{
	std::vector<Vertex> vertices;
	AllocatedBuffer vertexBuffer{ nullptr, nullptr };
	glm::vec3 objPosMin{ 0.0f };
	glm::vec3 objPosMax{ 0.0f };

	bool LoadFromObj(const char* filename);
	glm::vec3 GetObjectCenter() const;
};