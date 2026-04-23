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

  void AddYaw(Angle delta);
  void AddPitch(Angle delta);

private:
  void CreateDevice();

  void CreateSwapChainAndCmdList();

  void CreateIndexBuffer();
  void CreateVertexBuffers();

  void CreatePipeline();

  void CreateDescriptorHeap();

  void CreateConstantBuffer();
  void CreateTexture();

  std::vector<std::byte> ReadFile(std::filesystem::path path);

  HWND m_Hwnd;

  uint32_t m_ScreenWidth;
  uint32_t m_ScreenHeight;

  GltfModel m_Model;

  Mat4 m_ModelMat;
  Mat4 m_ProjMat;

  Angle m_Yaw;
  Angle m_Pitch;

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

  Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
  D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_PosBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_PosBufferView;

  DXGI_FORMAT m_PosVertexFormat;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_UvBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_UvBufferView;

  DXGI_FORMAT m_UvVertexFormat;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSig;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pipeline;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBuffer;

  D3D12_CPU_DESCRIPTOR_HANDLE m_CbvCpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE m_CbvGpuHandle;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;

  D3D12_CPU_DESCRIPTOR_HANDLE m_SrvCpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE m_SrvGpuHandle;

  Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
};