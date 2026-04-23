#include "GltfModel.h"

#include "external/nlohmann/json.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

#include <fstream>

using json = nlohmann::json;

static std::optional<GltfBuffer> ReadBuffer(int accessorIdx, const json& gltf, 
		std::filesystem::path dir) {
	GltfBuffer result;

	const auto& accessor = gltf["accessors"][accessorIdx];

	std::string_view vertexType = accessor["type"];

	int numComponents = 1;

	if (vertexType == "VEC3") {
		numComponents = 3;
	} else if (vertexType == "VEC2") {
		numComponents = 2;
	} else if (vertexType == "SCALAR") {
		numComponents = 1;
	} else {
		return std::nullopt;
	}

	int componentType = accessor["componentType"];

	int componentSize = 0;

	if (componentType == 5126) {
		componentSize = sizeof(float);
	} else if (componentType == 5123) {
		componentSize = sizeof(uint16_t);
	} else {
		return std::nullopt;
	}

	int vertCount = accessor["count"];

	int viewIdx = accessor["bufferView"];
	const auto& view = gltf["bufferViews"][viewIdx];

	int bufferIdx = view["buffer"];

	std::filesystem::path uri = gltf["buffers"][bufferIdx]["uri"];
	std::filesystem::path path = dir / uri;

	int offset = view["byteOffset"];
	int size = view["byteLength"];

	if (vertCount * numComponents * componentSize != size) {
		return std::nullopt;
	}

	result.Data.resize(size);

	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		return std::nullopt;
	}

	strm.seekg(offset);
	strm.read(reinterpret_cast<char*>(result.Data.data()), size);

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (componentType == 5126) {
		if (numComponents == 3) {
			format = DXGI_FORMAT_R32G32B32_FLOAT;

		} else if (numComponents == 2) {
			format = DXGI_FORMAT_R32G32_FLOAT;

		}	else {
			return std::nullopt;
		}

	} else if (componentType == 5123) {
		if (numComponents == 1) {
			format= DXGI_FORMAT_R16_UINT;
		} else {
			return std::nullopt;
		}
	} else {
		return std::nullopt;
	}

	result.Stride = numComponents * componentSize;
	result.Format = format;

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

	const json gltf = json::parse(strm);

	const auto& mesh = gltf["meshes"][0];
	const auto& primitive = mesh["primitives"][0];

	int indexIdx = primitive["indices"];

	const auto& attributes = primitive["attributes"];

	int posIdx = attributes["POSITION"];
	int uvIdx = attributes["TEXCOORD_0"];

	std::filesystem::path dir = path.parent_path();

	if (auto res = ReadBuffer(indexIdx, gltf, dir)) {
		result.IndexBuffer = *res;
	} else {
		return std::nullopt;
	}

	if (auto res = ReadBuffer(posIdx, gltf, dir)) {
		result.PosBuffer = *res;
	} else {
		return std::nullopt;
	}

	if (auto res = ReadBuffer(uvIdx, gltf, dir)) {
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