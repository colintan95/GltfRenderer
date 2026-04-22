#pragma once

#include "external/nlohmann/json.hpp"

#include <filesystem>
#include <vector>

template<typename T>
struct GltfBuffer {
	std::vector<T> Data;
	int NumComponents;
};

struct GltfModel {
	GltfBuffer<uint16_t> IdxBuffer;
	GltfBuffer<float> PosBuffer;
};

std::optional<GltfModel> LoadGltf(std::filesystem::path path);
