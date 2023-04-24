#pragma once

#include "asset_core.h"

namespace assets
{
	struct Vertex_PNCV_F32
	{
		float position[3];
		float normal[3];
		float color[3];
		float uv[2];
	};

	struct Vertex_P32N8C8V16
	{
		float position[3];
		uint8_t normal[3];
		uint8_t color[3];
		float uv[2];
	};


	enum class VertexFormat : uint32_t
	{
		Unknown = 0,
		PNCV_F32,
		P32N8C8V16
	};

	struct MeshBounds
	{
		float origin[3];
		float radius;
		float extents[3];
	};


	struct MeshInfo
	{
		uint64_t vertexBufferSize;
		uint64_t indexBufferSize;
		VertexFormat vertexFormat;
		MeshBounds bounds;
		uint8_t indexSize;
 		CompressionMode compressionMode;
		std::string sourceFile;
	};

	MeshInfo ReadMeshInfo(AssetFile* file);
	void UnpackMesh(MeshInfo* info, const char* sourceBuffer, size_t sourceSize, char* vertexBuffer, char* indexBuffer);
	AssetFile PackMesh(MeshInfo* info, void* vertexData, void* indexData);
	VertexFormat ParseVertexFormat(const char* string);
	MeshBounds CalculateBounds(const Vertex_PNCV_F32* verts, size_t count);
};