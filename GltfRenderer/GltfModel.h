#pragma once

#include <dxgiformat.h>

#include <filesystem>
#include <vector>

struct GltfBuffer {
	std::vector<std::byte> Data;

	int Stride;
	DXGI_FORMAT Format;
};

struct GltfImage {
	std::vector<std::byte> Data;

	int Width;
	int Height;

	int NumChannels;
};

struct GltfModel {
	GltfBuffer IndexBuffer;

	GltfBuffer PosBuffer;
	GltfBuffer UvBuffer;

	GltfImage Img;
};

std::optional<GltfModel> LoadGltf(std::filesystem::path path);
