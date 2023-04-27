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

	info.dataSize = textureMeta["bufferSize"];
	info.sourceFile = textureMeta["sourceFile"];

	for (auto& [key, value] : textureMeta["pages"].items())
	{
		assets::PageInfo page;

		page.width = value["width"];
		page.height = value["height"];
		page.compressedSize = value["compressedSize"];
		page.originalSize = value["originalSize"];

		info.pages.push_back(page);
	}

	return info;
}

void assets::UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination)
{
	if (info->compressionMode == CompressionMode::LZ4)
	{
		for (auto& page : info->pages)
		{
			LZ4_decompress_safe(sourceBuffer, destination, page.compressedSize, page.originalSize);
			sourceBuffer += page.compressedSize;
			destination += page.originalSize;
		}
	}
	else
	{
 		std::memcpy(destination, sourceBuffer, sourceSize);
	}
}

void assets::UnpackTexturePage(TextureInfo* info, int pageIndex, char* sourceBuffer, char* destination)
{
	char* source = sourceBuffer;
	for (int i = 0; i < pageIndex; ++i)
	{
		source += info->pages[i].compressedSize;
	}

	if (info->compressionMode == CompressionMode::LZ4)
	{
		if (info->pages[pageIndex].compressedSize != info->pages[pageIndex].originalSize)
			LZ4_decompress_safe(source, destination, info->pages[pageIndex].compressedSize, info->pages[pageIndex].originalSize);
		else
			std::memcpy(destination, source, info->pages[pageIndex].originalSize);
	}
	else
	{
		std::memcpy(destination, source, info->pages[pageIndex].originalSize);
	}
}

assets::AssetFile assets::PackTexture(TextureInfo* info, void* pixelData)
{
	constexpr CompressionMode compressMode = CompressionMode::LZ4;
	constexpr char* compressModeName = "LZ4";

	AssetFile file;
	file.type[0] = 'T';
	file.type[1] = 'X';
	file.type[2] = 'T';
	file.type[3] = 'R';
	file.version = TEXTURE_ASSET_VERSION;

	char* pixels = reinterpret_cast<char*>(pixelData);
	std::vector<char> pageBuffer;

	for (auto& page : info->pages)
	{
		pageBuffer.resize(page.originalSize);

		if (compressMode == CompressionMode::LZ4)
		{
			int compressStaging = LZ4_compressBound(static_cast<int>(page.originalSize));
			pageBuffer.resize(compressStaging);
			int compressedSize = LZ4_compress_default(pixels, pageBuffer.data(), page.originalSize, compressStaging);

			float compressionRate = static_cast<float>(compressedSize) / static_cast<float>(info->dataSize);

			if (compressionRate > 0.8)
			{
				compressedSize = page.originalSize;
				pageBuffer.resize(compressedSize);
				std::memcpy(pageBuffer.data(), pixels, compressedSize);
			}
			else
			{
				pageBuffer.resize(compressedSize);
			}

			page.compressedSize = compressedSize;
			file.blob.insert(file.blob.end(), pageBuffer.begin(), pageBuffer.end());

			pixels += page.originalSize;
		}
		else
		{
			std::cout << "Invalid texture file compression mode: '" << compressModeName << "'" << std::endl;
		}
	}

	nlohmann::json textureMeta;
	textureMeta["format"] = "RGBA8";
	textureMeta["bufferSize"] = info->dataSize;
	textureMeta["sourceFile"] = info->sourceFile;
	textureMeta["compression"] = compressModeName;

	std::vector<nlohmann::json> pageJson;
	for (auto& p : info->pages)
	{
		nlohmann::json page;
		page["compressedSize"] = p.compressedSize;
		page["originalSize"] = p.originalSize;
		page["width"] = p.width;
		page["height"] = p.height;
		pageJson.push_back(page);
	}
	textureMeta["pages"] = pageJson;

	file.json = textureMeta.dump();

	return file;
}

assets::TextureFormat assets::ParseTextureFormat(const char* string)
{
	if (strcmp(string, "RGBA8") == 0)
		return TextureFormat::RGBA8;
	return TextureFormat::Unknown;
}