#pragma once

#include "asset_core.h"

namespace assets
{
	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		RGBA8
	};

	struct PageInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t compressedSize;
		uint32_t originalSize;
	};

	struct TextureInfo
	{
		uint64_t dataSize;
		TextureFormat textureFormat;
 		CompressionMode compressionMode;
		std::string sourceFile;
		std::vector<PageInfo> pages;
	};

	TextureInfo ReadTextureInfo(AssetFile* file);
	void UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination);
	void UnpackTexturePage(TextureInfo* info, int pageIndex, char* sourceBuffer, char* destination);
	AssetFile PackTexture(TextureInfo* info, void* pixelData);
	TextureFormat ParseTextureFormat(const char* string);
};