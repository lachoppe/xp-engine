#include "vk_mesh.h"

#include <tiny_obj_loader.h>
#include <iostream>
#include "vk_engine.h"
#include "mesh_asset.h"
#include "debug.h"


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
	normalAttribute.format = VK_FORMAT_R8G8_UNORM; // was VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, octNormal);

	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R8G8B8_UNORM; // was VK_FORMAT_R32G32B32_SFLOAT;
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


glm::vec2 OctNormalWrap(glm::vec2 v)
{
	glm::vec2 wrap;
	wrap.x = (1.0f - glm::abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f);
	wrap.y = (1.0f - glm::abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f);
	return wrap;
}


glm::vec2 OctNormalEncode(glm::vec3 n)
{
	n /= (glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z));

	glm::vec2 wrapped = OctNormalWrap(n);

	glm::vec2 encoded;
	encoded.x = n.z >= 0.0f ? n.x : wrapped.x;
	encoded.y = n.z >= 0.0f ? n.y : wrapped.y;

	encoded.x = encoded.x * 0.5f + 0.5f;
	encoded.y = encoded.y * 0.5f + 0.5f;

	return encoded;
}


glm::vec3 OctNormalDecode(glm::vec2 encoded)
{
	encoded = encoded * 2.0f - 1.0f;

	glm::vec3 n = glm::vec3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));
	float t = glm::clamp(-n.z, 0.0f, 1.0f);
	n.x += n.x >= 0.0f ? -t : t;
	n.y += n.y >= 0.0f ? -t : t;

	n = glm::normalize(n);
	return n;
}


void Vertex::PackNormal(glm::vec3 n)
{
	glm::vec2 oct = OctNormalEncode(n);

	octNormal.x = uint8_t(oct.x * 255);
	octNormal.y = uint8_t(oct.y * 255);
}


void Vertex::PackColor(glm::vec3 c)
{
	color.r = static_cast<uint8_t>(c.x * 255);
	color.g = static_cast<uint8_t>(c.y * 255);
	color.b = static_cast<uint8_t>(c.z * 255);
}


bool Mesh::LoadFromAsset(const char* filename)
{
	assets::AssetFile asset;

	bool loaded = assets::LoadBinary(filename, asset);
	if (!loaded)
	{
		OutputMessage("Error loading mesh: %s", filename);
		return false;
	}

	assets::MeshInfo info = assets::ReadMeshInfo(&asset);

	std::vector<char> vertexBuffer;
	std::vector<char> indexBuffer;

	vertexBuffer.resize(info.vertexBufferSize);
	indexBuffer.resize(info.indexBufferSize);

	assets::UnpackMesh(&info, asset.blob.data(), asset.blob.size(), vertexBuffer.data(), indexBuffer.data());

	bounds.extents.x = info.bounds.extents[0];
	bounds.extents.y = info.bounds.extents[1];
	bounds.extents.z = info.bounds.extents[2];
	bounds.radius = info.bounds.radius;
	bounds.origin.x = info.bounds.origin[0];
	bounds.origin.y = info.bounds.origin[1];
	bounds.origin.z = info.bounds.origin[2];
	bounds.isValid = true;

	vertices.clear();
	indices.clear();

	indices.resize(indexBuffer.size() / sizeof(uint32_t));
	uint32_t* unpackedIndices = (uint32_t*)indexBuffer.data();
	for (int i = 0; i < indices.size(); ++i)
		indices[i] = unpackedIndices[i];

	if (info.vertexFormat == assets::VertexFormat::PNCV_F32)
	{
		assets::Vertex_PNCV_F32* unpackedVertices = (assets::Vertex_PNCV_F32*)vertexBuffer.data();

		vertices.resize(vertexBuffer.size() / sizeof(assets::Vertex_PNCV_F32));

		for (int i = 0; i < vertices.size(); ++i)
		{
			vertices[i].position.x = unpackedVertices[i].position[0];
			vertices[i].position.y = unpackedVertices[i].position[1];
			vertices[i].position.z = unpackedVertices[i].position[2];

			glm::vec3 normal = glm::vec3(
				unpackedVertices[i].normal[0],
				unpackedVertices[i].normal[1],
				unpackedVertices[i].normal[2]);
			vertices[i].PackNormal(normal);

			glm::vec3 color = glm::vec3(
				unpackedVertices[i].color[0],
				unpackedVertices[i].color[1],
				unpackedVertices[i].color[2]);
			vertices[i].PackColor(color);

			vertices[i].uv.x = unpackedVertices[i].uv[0];
			vertices[i].uv.y = unpackedVertices[i].uv[1];
		}
	}
	else if (info.vertexFormat == assets::VertexFormat::P32N8C8V16)
	{
		assets::Vertex_P32N8C8V16* unpackedVertices = (assets::Vertex_P32N8C8V16*)vertexBuffer.data();

		vertices.resize(vertexBuffer.size() / sizeof(assets::Vertex_P32N8C8V16));

		for (int i = 0; i < vertices.size(); ++i)
		{
			vertices[i].position.x = unpackedVertices[i].position[0];
			vertices[i].position.y = unpackedVertices[i].position[1];
			vertices[i].position.z = unpackedVertices[i].position[2];

			glm::vec3 normal = glm::vec3(
				unpackedVertices[i].normal[0] / 255.0f * 2.0f - 1.0f,
				unpackedVertices[i].normal[1] / 255.0f * 2.0f - 1.0f,
				unpackedVertices[i].normal[2] / 255.0f * 2.0f - 1.0f);
			vertices[i].PackNormal(normal);

			glm::vec3 color = glm::vec3(
				unpackedVertices[i].color[0],
				unpackedVertices[i].color[1],
				unpackedVertices[i].color[2]);
			vertices[i].PackColor(color);

			vertices[i].uv.x = unpackedVertices[i].uv[0];
			vertices[i].uv.y = unpackedVertices[i].uv[1];
		}
	}

	// log success

	return true;
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
				glm::vec3 normal = glm::vec3(
					nx / 255.0f * 2.0f - 1.0f,
					ny / 255.0f * 2.0f - 1.0f,
					nz / 255.0f * 2.0f - 1.0f);
				newVert.PackNormal(normal);
// 				newVert.normal.x = nx;
// 				newVert.normal.y = ny;
// 				newVert.normal.z = nz;
				newVert.uv.x = ux;
				newVert.uv.y = 1.0f - uy;	// Vulkan V is flipped from OBJ

				// DEBUG normal as color
// 				newVert.color = newVert.normal;
				newVert.color.r = 1.0f;
				newVert.color.g = 1.0f;
				newVert.color.b = 1.0f;

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