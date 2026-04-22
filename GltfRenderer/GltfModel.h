#pragma once

#include <filesystem>
#include <vector>

template<typename T>
struct GltfBuffer {
	std::vector<T> Data;
	int NumComponents;
};

struct GltfImage {
	std::vector<std::byte> Data;

	int Width;
	int Height;

	int NumChannels;
};

struct GltfModel {
	GltfBuffer<uint16_t> IdxBuffer;

	GltfBuffer<float> PosBuffer;
	GltfBuffer<float> UvBuffer;

	GltfImage Img;
};

std::optional<GltfModel> LoadGltf(std::filesystem::path path);
