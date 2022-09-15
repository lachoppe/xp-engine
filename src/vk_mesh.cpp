#include "vk_mesh.h"
#include "vk_engine.h"

#include <tiny_obj_loader.h>
#include <iostream>


VertexInputDescription Vertex::GetVertexDescription()
{
	VertexInputDescription description;

	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, color);

	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.location = 3;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.offset = offsetof(Vertex, uv);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(colorAttribute);
	description.attributes.push_back(uvAttribute);

	return description;
}


bool Mesh::LoadFromObj(const char* filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr);

	if (!warn.empty())
	{
		OutputMessage("[WARN] Loading '%s': %s\n", filename, warn.c_str());
	}

	if (!err.empty())
	{
		OutputMessage("[ERROR] Loading '%s': %s\n", filename, err.c_str());
		return false;
	}

	const bool hasNormals = attrib.normals.size() > 0;
	const bool hasUVs = attrib.texcoords.size() > 0;
	// Fallback values
	tinyobj::real_t nx = 0.0f;
	tinyobj::real_t ny = 0.0f;
	tinyobj::real_t nz = 0.0f;
	tinyobj::real_t ux = 0.0f;
	tinyobj::real_t uy = 0.0f;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_t indexOffset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			// Hard-coding only loading to triangles
			const int fv = 3;

			for (size_t v = 0; v < fv; v++)
			{
				tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

				if (hasNormals)
				{
					nx = attrib.normals[3 * idx.normal_index + 0];
					ny = attrib.normals[3 * idx.normal_index + 1];
					nz = attrib.normals[3 * idx.normal_index + 2];
				}

				if (hasUVs)
				{
					ux = attrib.texcoords[2 * idx.texcoord_index + 0];
					uy = attrib.texcoords[2 * idx.texcoord_index + 1];
				}

				Vertex newVert;
				newVert.position.x = vx;
				newVert.position.y = vy;
				newVert.position.z = vz;
				newVert.normal.x = nx;
				newVert.normal.y = ny;
				newVert.normal.z = nz;
				newVert.uv.x = ux;
				newVert.uv.y = 1.0f - uy;	// Vulkan V is flipped from OBJ

				// DEBUG normal as color
				newVert.color = newVert.normal;

				// Update object bounding box
				objPosMin = glm::min(objPosMin, newVert.position);
				objPosMax = glm::max(objPosMax, newVert.position);

				vertices.push_back(newVert);
			}

			indexOffset += 3;
		}
	}

	return true;
}


glm::vec3 Mesh::GetObjectCenter() const
{
	return (objPosMin + objPosMax) * 0.5f;
}