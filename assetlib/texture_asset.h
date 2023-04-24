#pragma once

#include "asset_core.h"

namespace assets
{
	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		RGBA8
	};

	struct TextureInfo
	{
		uint64_t dataSize;
		TextureFormat textureFormat;
 		CompressionMode compressionMode;
		uint32_t resolution[3];
		std::string sourceFile;
	};

	TextureInfo ReadTextureInfo(AssetFile* file);
	void UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination);
	AssetFile PackTexture(TextureInfo* info, void* pixelData);
	TextureFormat ParseTextureFormat(const char* string);
};