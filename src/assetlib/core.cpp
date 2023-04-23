#include "core.h"
#include "../debug.h"

#include <fstream>

#define ASSET_VERSION 1


bool assets::SaveBinary(const char* path, const AssetFile& file)
{
	std::ofstream outFile;
 	outFile.open(path, std::ofstream::binary | std::ofstream::trunc);
	if (!outFile.is_open())
	{
		OutputMessage("ERROR: Asset: failed to open for writing: %s", path);
		return false;
	}

	outFile.write(reinterpret_cast<const char*>(&file.type), 4);

	uint32_t assetVersion = ASSET_VERSION << 16;
	uint32_t contentVersion = file.version & 0x0000ffff;
	uint32_t versionPacked = assetVersion | contentVersion;
	outFile.write(reinterpret_cast<const char*>(&versionPacked), sizeof(uint32_t));

	uint32_t jsonLen = static_cast<uint32_t>(file.json.size());
	outFile.write(reinterpret_cast<const char*>(&jsonLen), sizeof(uint32_t));
	uint32_t blobLen = static_cast<uint32_t>(file.blob.size());
	outFile.write(reinterpret_cast<const char*>(&blobLen), sizeof(uint32_t));

	outFile.write(file.json.data(), jsonLen);
	outFile.write(file.blob.data(), blobLen);

	outFile.close();

	return true;
}

bool assets::LoadBinary(const char* path, AssetFile& asset)
{
	std::ifstream inFile;
	inFile.open(path, std::ifstream::binary);

	if (!inFile.is_open())
		return false;

	inFile.seekg(0);

	inFile.read(asset.type, 4);

	uint32_t versionPacked = 0;
	inFile.read(reinterpret_cast<char*>(&versionPacked), sizeof(uint32_t));
	uint32_t assetVersion = (versionPacked >> 16);
	if (assetVersion != ASSET_VERSION)
	{
		OutputMessage("ERROR: Asset: Invalid version: read %d, need %d\nPath: %s\n", assetVersion, ASSET_VERSION, path);
		return false;
	}
	asset.version = versionPacked & 0x0000ffff;

	uint32_t jsonLen = 0;
	inFile.read(reinterpret_cast<char*>(&jsonLen), sizeof(uint32_t));
	uint32_t blobLen = 0;
	inFile.read(reinterpret_cast<char*>(&blobLen), sizeof(uint32_t));

	asset.json.resize(jsonLen);
	inFile.read(asset.json.data(), jsonLen);
	asset.blob.resize(blobLen);
	inFile.read(asset.blob.data(), blobLen);

	return true;
}

assets::CompressionMode assets::ParseCompression(const char* string)
{
	if (strcmp(string, "LZ4") == 0)
		return CompressionMode::LZ4;
	return CompressionMode::None;
}