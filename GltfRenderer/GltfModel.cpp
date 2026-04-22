#include "GltfModel.h"

#include <fstream>

using json = nlohmann::json;

template<typename T>
std::optional<std::vector<T>> ReadBufferData(int accessorIdx, int numComponents, const json& gltf, 
		std::filesystem::path dir) {
	std::vector<T> result;

	const auto& accessor = gltf["accessors"][accessorIdx];

	int viewIdx = accessor["bufferView"];
	const auto& view = gltf["bufferViews"][viewIdx];

	int bufferIdx = view["buffer"];

	std::filesystem::path uri = gltf["buffers"][bufferIdx]["uri"];
	std::filesystem::path path = dir / uri;

	int offset = view["byteOffset"];
	int size = view["byteLength"];

	int vertCount = accessor["count"];

	if (vertCount * numComponents * sizeof(T) != size) {
		return std::nullopt;
	}

	result.resize(size);

	std::ifstream strm(path, std::ios::binary);
	if (!strm.is_open()) {
		return std::nullopt;
	}

	strm.seekg(offset);
	strm.read(reinterpret_cast<char*>(result.data()), size);

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

	if (auto res = ReadBufferData<uint16_t>(0, 1, gltf, dir)) {
		result.IdxBuffer = {
			.Data = *res,
			.NumComponents = 1
		};
	} else {
		return std::nullopt;
	}

	if (auto res = ReadBufferData<float>(1, 3, gltf, dir)) {
		result.PosBuffer = {
			.Data = *res,
			.NumComponents = 3
		};
	} else {
		return std::nullopt;
	}

	return result;
}