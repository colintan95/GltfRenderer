#pragma once

#include "GltfModel.h"
#include "Math.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <combaseapi.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>
#include <vector>

class App {
public:
  bool Init(HWND hwnd);

  void Render();

private:
  void CreateDevice();

  void CreateSwapChainAndCmdList();

  void CreatePipeline();

  void CreateDescriptorHeap();

  void CreateIndexBuffer();
  void CreateVertexBuffers();

  void CreateConstantBuffer();
  void CreateTexture();

  std::vector<std::byte> ReadFile(std::filesystem::path path);

  GltfModel m_Model;
  Mat4 m_MvpMat;

  HWND m_Hwnd;

  uint32_t m_ScreenWidth;
  uint32_t m_ScreenHeight;

  static constexpr uint8_t kFrameCount = 2;

  uint8_t m_FrameIdx;

  Microsoft::WRL::ComPtr<IDXGIFactory6> m_Factory;
  Microsoft::WRL::ComPtr<ID3D12Device> m_Device;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
  uint32_t m_RtvHandleSize;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[kFrameCount];

  Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
  uint64_t m_FenceValue;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CmdQueue;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAlloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSig;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pipeline;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_IdxBuffer;
  D3D12_INDEX_BUFFER_VIEW m_IdxBufferView;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_PosBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_PosBufferView;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_UvBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_UvBufferView;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBuffer;

  D3D12_CPU_DESCRIPTOR_HANDLE m_CbvCpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE m_CbvGpuHandle;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;

  D3D12_CPU_DESCRIPTOR_HANDLE m_SrvCpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE m_SrvGpuHandle;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
};