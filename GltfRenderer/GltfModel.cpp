#include "GltfModel.h"

#include "external/nlohmann/json.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

#include <fstream>

using json = nlohmann::json;

template<typename T>
std::optional<GltfBuffer<T>> ReadBuffer(int accessorIdx, int numComponents, const json& gltf,
		std::filesystem::path dir) {
	GltfBuffer<T> result;

	const auto& accessor = gltf["accessors"][accessorIdx];
	int vertCount = accessor["count"];

	int viewIdx = accessor["bufferView"];
	const auto& view = gltf["bufferViews"][viewIdx];

	int bufferIdx = view["buffer"];

	std::filesystem::path uri = gltf["buffers"][bufferIdx]["uri"];
	std::filesystem::path path = dir / uri;

	int offset = view["byteOffset"];
	int size = view["byteLength"];

	if (vertCount * numComponents * sizeof(T) != size) {
		return std::nullopt;
	}

	result.Data.resize(vertCount * numComponents);

	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		return std::nullopt;
	}

	strm.seekg(offset);
	strm.read(reinterpret_cast<char*>(result.Data.data()), size);

	result.NumComponents = numComponents;

	return result;
}

static std::optional<GltfImage> ReadImage(int imageIdx, const json& gltf, 
		std::filesystem::path dir) {
	GltfImage result;

	const auto& image = gltf["images"][imageIdx];

	std::filesystem::path uri = image["uri"];
	std::filesystem::path path = dir / uri;

	int width = 0;
	int height = 0;

	static constexpr int kNumChannels = 4;

	unsigned char* data = stbi_load(path.string().c_str(), &width, &height, nullptr, kNumChannels);
	if (data == nullptr) {
		return std::nullopt;
	}

	int size = width * height * kNumChannels;

	// TODO: See if we can do this without a copy.
	result.Data.resize(size);
	memcpy(result.Data.data(), data, size);
	
	stbi_image_free(data);

	result.Width = width;
	result.Height = height;

	result.NumChannels = kNumChannels;

	return result;
}

std::optional<GltfModel> LoadGltf(std::filesystem::path path) {
	GltfModel result;

	std::ifstream strm(path);
	if (!strm.is_open()) {
		return std::nullopt;
	}

	json gltf = json::parse(strm);

	std::filesystem::path dir = path.parent_path();

	if (auto res = ReadBuffer<uint16_t>(0, 1, gltf, dir)) {
		result.IdxBuffer = *res;
	} else {
		return std::nullopt;
	}

	if (auto res = ReadBuffer<float>(1, 3, gltf, dir)) {
		result.PosBuffer = *res;
	} else {
		return std::nullopt;
	}

	if (auto res = ReadBuffer<float>(4, 2, gltf, dir)) {
		result.UvBuffer = *res;
	}
	else {
		return std::nullopt;
	}

	if (auto res = ReadImage(0, gltf, dir)) {
		result.Img = *res;
	} else {
		return std::nullopt;
	}

	return result;
}