#include "mesh_asset.h"

#include <iostream>
#include <cstring>
#include <vector>
#include "json.hpp"
#include "lz4.h"

#define MESH_ASSET_VERSION 1

assets::MeshInfo assets::ReadMeshInfo(AssetFile* file)
{
	MeshInfo info;

	nlohmann::json meshMeta = nlohmann::json::parse(file->json);

	std::string formatStr = meshMeta["format"];
	info.vertexFormat = assets::ParseVertexFormat(formatStr.c_str());

	std::string compressionModeStr = meshMeta["compression"];
	info.compressionMode = assets::ParseCompression(compressionModeStr.c_str());

	info.vertexBufferSize = meshMeta["vbSize"];
	info.indexBufferSize = meshMeta["ibSize"];
	info.indexSize = static_cast<uint8_t>(meshMeta["indexSize"]);
	info.sourceFile = meshMeta["sourceFile"];

	std::vector<float> boundsData;
	boundsData.reserve(sizeof(MeshBounds) / sizeof(float));
	boundsData = meshMeta["bounds"].get<std::vector<float>>();
	info.bounds.origin[0] = boundsData[0];
	info.bounds.origin[1] = boundsData[1];
	info.bounds.origin[2] = boundsData[2];
	info.bounds.radius = boundsData[3];
	info.bounds.extents[0] = boundsData[4];
	info.bounds.extents[1] = boundsData[5];
	info.bounds.extents[2] = boundsData[6];

	return info;
}

void assets::UnpackMesh(MeshInfo* info, const char* sourceBuffer, size_t sourceSize, char* vertexBuffer, char* indexBuffer)
{
	if (info->compressionMode == CompressionMode::LZ4)
	{
		std::vector<char> decompressBuffer;
		decompressBuffer.resize(info->vertexBufferSize + info->indexBufferSize);
		LZ4_decompress_safe(sourceBuffer, decompressBuffer.data(), static_cast<int>(sourceSize), static_cast<int>(decompressBuffer.size()));

		memcpy(vertexBuffer, decompressBuffer.data(), info->vertexBufferSize);
		memcpy(indexBuffer, decompressBuffer.data() + info->vertexBufferSize, info->indexBufferSize);
	}
}

assets::AssetFile assets::PackMesh(MeshInfo* info, void* vertexData, void* indexData)
{
	constexpr CompressionMode compressMode = assets::CompressionMode::LZ4;
	constexpr char* compressModeName = "LZ4";

	const char* vertexFormat;
	switch (info->vertexFormat)
	{
	case assets::VertexFormat::P32N8C8V16:
		vertexFormat = "P32N8C8V16";
		break;
	case assets::VertexFormat::PNCV_F32:
		vertexFormat = "PNCV_F32";
		break;
	default:
		vertexFormat = "unknown";
	}

	nlohmann::json meshMeta;
	meshMeta["vbSize"] = info->vertexBufferSize;
	meshMeta["ibSize"] = info->indexBufferSize;
	meshMeta["format"] = vertexFormat;
	meshMeta["indexSize"] = info->indexSize;
	meshMeta["compression"] = compressModeName;
	meshMeta["sourceFile"] = info->sourceFile;

	std::vector<float> boundsData;
	boundsData.reserve(sizeof(MeshBounds) / sizeof(float));
	boundsData.push_back(info->bounds.origin[0]);
	boundsData.push_back(info->bounds.origin[1]);
	boundsData.push_back(info->bounds.origin[2]);
	boundsData.push_back(info->bounds.radius);
	boundsData.push_back(info->bounds.extents[0]);
	boundsData.push_back(info->bounds.extents[1]);
	boundsData.push_back(info->bounds.extents[2]);
	meshMeta["bounds"] = boundsData;

	AssetFile file;
	file.type[0] = 'M';
	file.type[1] = 'E';
	file.type[2] = 'S';
	file.type[3] = 'H';
	file.version = MESH_ASSET_VERSION;

	file.json = meshMeta.dump();


	size_t dataSize = info->vertexBufferSize + info->indexBufferSize;
	std::vector<char> mergedBuffer;
	mergedBuffer.resize(dataSize);

	std::memcpy(mergedBuffer.data(), vertexData, info->vertexBufferSize);
	std::memcpy(mergedBuffer.data() + info->vertexBufferSize, indexData, info->indexBufferSize);

	if (compressMode == CompressionMode::LZ4)
	{
		int compressStaging = LZ4_compressBound(static_cast<int>(dataSize));
		file.blob.resize(compressStaging);
		int compressedSize = LZ4_compress_default(mergedBuffer.data(), file.blob.data(), static_cast<int>(mergedBuffer.size()), compressStaging);
		file.blob.resize(compressedSize);
	}
	else
	{
		std::cout << "Invalid mesh file compression mode: '" << compressModeName << "'" << std::endl;
	}

	return file;
}

assets::VertexFormat assets::ParseVertexFormat(const char* string)
{
	if (strcmp(string, "P32N8C8V16") == 0)
		return assets::VertexFormat::P32N8C8V16;
	if (strcmp(string, "PNCV_F32") == 0)
		return assets::VertexFormat::PNCV_F32;
	return assets::VertexFormat::Unknown;
}

assets::MeshBounds assets::CalculateBounds(const Vertex_PNCV_F32* verts, size_t count)
{
	MeshBounds bounds{};

	float min[3] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	float max[3] = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

	for (int i = 0; i < count; ++i)
	{
		min[0] = std::min(min[0], verts[i].position[0]);
		min[1] = std::min(min[1], verts[i].position[1]);
		min[2] = std::min(min[2], verts[i].position[2]);

		max[0] = std::max(max[0], verts[i].position[0]);
		max[1] = std::max(max[1], verts[i].position[1]);
		max[2] = std::max(max[2], verts[i].position[2]);
	}

	bounds.extents[0] = (max[0] - min[0]) * 0.5f;
	bounds.extents[1] = (max[1] - min[1]) * 0.5f;
	bounds.extents[2] = (max[2] - min[2]) * 0.5f;

	bounds.origin[0] = bounds.extents[0] + min[0];
	bounds.origin[1] = bounds.extents[1] + min[1];
	bounds.origin[2] = bounds.extents[2] + min[2];

	float rsq = 0.0f;
	for (int i = 0; i < count; ++i)
	{
		float offset[3] = {
			verts[i].position[0] - bounds.origin[0],
			verts[i].position[1] - bounds.origin[1],
			verts[i].position[2] - bounds.origin[2]
		};

		float distsq = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
		rsq = std::max(distsq, rsq);
	}
	bounds.radius = std::sqrt(rsq);
	
	return bounds;
}
