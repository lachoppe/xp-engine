#include "material_asset.h"
#include "json.hpp"
#include "lz4.h"

#define MATERIAL_ASSET_VERSION 1

using namespace assets;


MaterialInfo assets::ReadMaterialInfo(AssetFile* asset)
{
	MaterialInfo info{};

	nlohmann::json materialMeta = nlohmann::json::parse(asset->json);
	info.baseEffect = materialMeta["baseEffect"];

	for (auto& [key, value] : materialMeta["textures"].items())
		info.textures[key] = value;

	for (auto& [key, value] : materialMeta["customProperties"].items())
		info.customProps[key] = value;

	info.transparency = TransparencyMode::Opaque;

	auto it = materialMeta.find("transparency");
	if (it != materialMeta.end())
	{
		std::string value = *it;
		if (value.compare("transparent") == 0)
			info.transparency = TransparencyMode::Transparent;
		else if (value.compare("masked") == 0)
			info.transparency = TransparencyMode::Masked;
	}

	return info;
}


AssetFile assets::PackMaterial(MaterialInfo* info)
{
	nlohmann::json materialMeta;

	materialMeta["baseEffect"] = info->baseEffect;
	materialMeta["textures"] = info->textures;
	materialMeta["customProperties"] = info->customProps;

	switch (info->transparency)
	{
	case TransparencyMode::Transparent:
		materialMeta["transparency"] = "transparent";
		break;
	case TransparencyMode::Masked:
		materialMeta["transparency"] = "masked";
		break;
	}

	AssetFile asset;
	asset.type[0] = 'M';
	asset.type[1] = 'T';
	asset.type[2] = 'R';
	asset.type[3] = 'L';
	asset.version = MATERIAL_ASSET_VERSION;

	std::string stringified = materialMeta.dump();
	asset.json = stringified;

	return asset;
}
