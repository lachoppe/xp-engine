#include <iostream>
// #include <fstream>
#include <filesystem>
#include <json.hpp>
#include <lz4.h>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tiny_obj_loader.h"

#include "asset_core.h"
#include "texture_asset.h"
#include "mesh_asset.h"

// #define TINYGLTF_IMPLEMENTATION
// #include <tiny_gltf.h>
// #include <nvtt.h>

// #include <glm/glm.hpp>
// #include <glm/gtx/transform.hpp>
// #include <glm/gtx/quaternion.hpp>

constexpr const char* INDENT = "    ";
constexpr const char* OUTPUT_FOLDER = "cooked";
constexpr bool TIMINGS = true;

#define START_TIMING(var) \
	auto _##var##Start = timer::high_resolution_clock::now();

#define END_TIMING(name, var) \
	auto _##var##Diff = timer::high_resolution_clock::now() - _##var##Start; \
	if (TIMINGS) { std::cout << INDENT << name << timer::duration_cast<timer::nanoseconds>(_##var##Diff).count() / 1000000.0 << "ms" << std::endl; }

namespace fs = std::filesystem;
namespace timer = std::chrono;
using namespace assets;


bool ConvertImage(const fs::path& inPath, const fs::path& outPath)
{
	int width, height, channels;

	START_TIMING(load)
	stbi_uc* pixels = stbi_load(inPath.u8string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
	END_TIMING("    PNG load", load)

	if (!pixels)
	{
		std::cout << "Failed to load texture file: %s" << inPath << std::endl;
		return false;
	}

	TextureInfo info{};
	info.dataSize = static_cast<uint64_t>(width) * height * 4;		// loaded as RGBA
	info.resolution[0] = width;
	info.resolution[1] = height;
	info.textureFormat = TextureFormat::RGBA8;
	info.sourceFile = inPath.string();

// 	auto packStart = timer::high_resolution_clock::now();
	AssetFile asset = PackTexture(&info, pixels);
// 	auto packDiff = timer::high_resolution_clock::now() - packStart;
// 	PRINT_TIMING_DIFF("PNG pack: ", packDiff)

	stbi_image_free(pixels);

	return SaveBinary(outPath.u8string().c_str(), asset);
}

void PackVertex(Vertex_PNCV_F32& newVert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t u, tinyobj::real_t v)
{
	newVert.position[0] = vx;
	newVert.position[1] = vy;
	newVert.position[2] = vz;
	newVert.normal[0] = nx;
	newVert.normal[1] = ny;
	newVert.normal[2] = nz;
	newVert.uv[0] = u;
	newVert.uv[1] = 1.0f - v;	// Vulkan V is flipped from OBJ
}

void PackVertex(Vertex_P32N8C8V16& newVert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t u, tinyobj::real_t v)
{
	newVert.position[0] = vx;
	newVert.position[1] = vy;
	newVert.position[2] = vz;
	newVert.normal[0] = static_cast<uint8_t>(((nx + 1.0) / 2.0) * 255);
	newVert.normal[1] = static_cast<uint8_t>(((ny + 1.0) / 2.0) * 255);
	newVert.normal[2] = static_cast<uint8_t>(((nz + 1.0) / 2.0) * 255);
	newVert.uv[0] = u;
	newVert.uv[1] = 1.0f - v;	// Vulkan V is flipped from OBJ
}

template<typename V, typename I>
void ExtractMeshFromObj(const std::vector<tinyobj::shape_t>& shapes, const tinyobj::attrib_t& attrib, std::vector<I>& indices, std::vector<V>& vertices)
{
	const bool hasNormals = attrib.normals.size() > 0;
	const bool hasUVs = attrib.texcoords.size() > 0;

	// Fallback values
	tinyobj::real_t nx = 0.0f;
	tinyobj::real_t ny = 1.0f;
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

				V newVert;
				PackVertex(newVert, vx, vy, vz, nx, ny, nz, ux, uy);

				// Exploded mesh, so indices are just an ascending count
				indices.push_back(static_cast<I>(vertices.size()));

				vertices.push_back(newVert);
			}

			indexOffset += 3;
		}
	}
}

bool ConvertMesh(const fs::path& inPath, const fs::path& outPath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	fs::path mtlPath = inPath;
	mtlPath.remove_filename();

// 	auto loadStart = timer::high_resolution_clock::now();
	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inPath.u8string().c_str(), mtlPath.u8string().c_str());
// 	auto loadDiff = timer::high_resolution_clock::now() - loadStart;
// 	PRINT_TIMING_DIFF("OBJ load: ", loadDiff)

	if (!warn.empty())
	{
		std::cout << INDENT << INDENT << "Mesh load warning: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << INDENT << INDENT << "Mesh load error: %s" << warn << std::endl;
		return false;
	}

	using IndexFormat = uint32_t;
	using VertexFormat = assets::Vertex_PNCV_F32;
	auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;

	std::vector<IndexFormat> indices;
	std::vector<VertexFormat> vertices;
	ExtractMeshFromObj<VertexFormat, IndexFormat>(shapes, attrib, indices, vertices);

	MeshInfo info{};
	info.vertexFormat = VertexFormatEnum;
	info.vertexBufferSize = vertices.size() & sizeof(VertexFormat);
	info.indexBufferSize = indices.size() & sizeof(IndexFormat);
	info.indexSize = sizeof(IndexFormat);
	info.sourceFile = inPath.string();

	info.bounds = CalculateBounds(vertices.data(), vertices.size());

// 	auto packStart = timer::high_resolution_clock::now();
	AssetFile asset = PackMesh(&info, vertices.data(), indices.data());
// 	auto packDiff = timer::high_resolution_clock::now() - packStart;
// 	PRINT_TIMING_DIFF("OBJ pack: ", packDiff)


	return SaveBinary(outPath.u8string().c_str(), asset);
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " <asset_folder>";

		std::cout << std::endl << "Press enter to continue...";
		int a = std::getchar();

		return -1;
	}

	fs::path path{ argv[1] };

	fs::path directory = path;

	fs::path exportDir = path.parent_path() / OUTPUT_FOLDER;


	std::cout << "Processing asset directory at " << directory << std::endl;

// 	auto cookStart = timer::high_resolution_clock::now();

	for (auto& p : fs::recursive_directory_iterator(directory))
	{
		std::cout << INDENT << "File: " << p << ": ";

		auto relative = p.path().lexically_proximate(directory);
		auto exportPath = exportDir / relative;

		if (!fs::is_directory(exportPath.parent_path()))
			fs::create_directory(exportPath.parent_path());

		if (p.path().extension() == ".png")
		{
			std::cout << " converting texture..." << std::endl;

			auto newPath = exportPath;
			newPath.replace_extension(".tex");
			if (!ConvertImage(p.path(), newPath))
				std::cout << " done." << std::endl;
		}
		else if (p.path().extension() == ".obj")
		{
			std::cout << " converting mesh..." << std::endl;

			auto newPath = exportPath;
			newPath.replace_extension(".msh");
			if (!ConvertMesh(p.path(), newPath))
				std::cout << " done." << std::endl;
		}
		else
		{
			std::cout << " skipping." << std::endl;
		}
	}

// 	auto cookDiff = timer::high_resolution_clock::now() - cookStart;
// 	PRINT_TIMING_DIFF("COOK: ", cookDiff)

	std::cout << std::endl << "Press enter to continue...";
	int a = std::getchar();

	return 0;
}