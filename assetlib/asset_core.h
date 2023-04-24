#pragma once

#include <string>
#include <vector>

namespace assets
{
	enum CompressionMode : uint32_t
	{
		None = 0,
		LZ4
	};

	struct AssetFile {
		char type[4];
		uint32_t version;
		std::string json;
		std::vector<char> blob;
	};

	bool SaveBinary(const char* path, const AssetFile& file);
	bool LoadBinary(const char* path, AssetFile& asset);
	CompressionMode ParseCompression(const char* string);
}