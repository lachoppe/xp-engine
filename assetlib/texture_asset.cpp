#include "texture_asset.h"

#include <iostream>
#include <cstring>
#include "json.hpp"
#include "lz4.h"

#define TEXTURE_ASSET_VERSION 1

assets::TextureInfo assets::ReadTextureInfo(AssetFile* file)
{
	TextureInfo info;

	nlohmann::json textureMeta = nlohmann::json::parse(file->json);

	std::string formatStr = textureMeta["format"];
	info.textureFormat = ParseTextureFormat(formatStr.c_str());

	std::string compressionModeStr = textureMeta["compression"];
	info.compressionMode = assets::ParseCompression(compressionModeStr.c_str());

	info.resolution[0] = textureMeta["width"];
	info.resolution[1] = textureMeta["height"];
	info.dataSize = textureMeta["bufferSize"];
	info.sourceFile = textureMeta["sourceFile"];

	return info;
}

void assets::UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination)
{
	if (info->compressionMode == CompressionMode::LZ4)
	{
		LZ4_decompress_safe(sourceBuffer, destination, static_cast<int>(sourceSize), static_cast<int>(info->dataSize));
	}
	else
	{
 		std::memcpy(destination, sourceBuffer, sourceSize);
	}
}

assets::AssetFile assets::PackTexture(TextureInfo* info, void* pixelData)
{
	constexpr CompressionMode compressMode = CompressionMode::LZ4;
	constexpr char* compressModeName = "LZ4";

	nlohmann::json textureMeta;
	textureMeta["format"] = "RGBA8";
	textureMeta["width"] = info->resolution[0];
	textureMeta["height"] = info->resolution[1];
	textureMeta["bufferSize"] = info->dataSize;
	textureMeta["sourceFile"] = info->sourceFile;
	textureMeta["compression"] = compressModeName;

	AssetFile file;
	file.type[0] = 'T';
	file.type[1] = 'X';
	file.type[2] = 'T';
	file.type[3] = 'R';
	file.version = TEXTURE_ASSET_VERSION;

	file.json = textureMeta.dump();

	if (compressMode == CompressionMode::LZ4)
	{
		int compressStaging = LZ4_compressBound(static_cast<int>(info->dataSize));
		file.blob.resize(compressStaging);
		int compressedSize = LZ4_compress_default((const char*)pixelData, file.blob.data(), static_cast<int>(info->dataSize), compressStaging);
		file.blob.resize(compressedSize);
	}
	else
	{
		std::cout << "Invalid texture file compression mode: '" << compressModeName << "'" << std::endl;
	}

	return file;
}

assets::TextureFormat assets::ParseTextureFormat(const char* string)
{
	if (strcmp(string, "RGBA8") == 0)
		return TextureFormat::RGBA8;
	return TextureFormat::Unknown;
}