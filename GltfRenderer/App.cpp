#include "App.h"

#include <fstream>

using Microsoft::WRL::ComPtr;

App::App(HWND hwnd) {
  m_Hwnd = hwnd;

  RECT wndRect = {};
  GetClientRect(m_Hwnd, &wndRect);

  m_ScreenWidth = wndRect.right;
  m_ScreenHeight = wndRect.bottom;

  CreateDevice();

  CreateSwapChainAndCmdList();

  CreatePipeline();

  CreateVertexBuffer();
}

void App::Render() {
  if (m_PresentFence->GetCompletedValue() < m_PresentFenceValue) {
    return;
  }

  m_FrameIdx = m_SwapChain->GetCurrentBackBufferIndex();

  m_CmdAlloc->Reset();
  m_CmdList->Reset(m_CmdAlloc.Get(), m_Pipeline.Get());

  m_CmdList->SetGraphicsRootSignature(m_RootSig.Get());

  D3D12_VIEWPORT viewport = {
    .TopLeftX = 0,
    .TopLeftY = 0,
    .Width = static_cast<float>(m_ScreenWidth),
    .Height = static_cast<float>(m_ScreenHeight),
    .MinDepth = D3D12_MIN_DEPTH,
    .MaxDepth = D3D12_MAX_DEPTH
  };

  m_CmdList->RSSetViewports(1, &viewport);

  D3D12_RECT scissor = {
    .left = 0,
    .top = 0,
    .right = static_cast<LONG>(m_ScreenWidth),
    .bottom = static_cast<LONG>(m_ScreenHeight)
  };

  m_CmdList->RSSetScissorRects(1, &scissor);

  auto* target = m_RenderTargets[m_FrameIdx].Get();

  D3D12_RESOURCE_BARRIER renderBarrier = {
    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
    .Transition = {
      .pResource = target,
      .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
      .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
      .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
    }
  };

  m_CmdList->ResourceBarrier(1, &renderBarrier);

  auto rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
  rtvHandle.ptr += m_FrameIdx * m_RtvHandleSize;

  m_CmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

  float color[] = { 0.f, 0.f, 0.f, 1.f };
  m_CmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);

  m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_CmdList->IASetVertexBuffers(0, 1, &m_VertBufferView);

  m_CmdList->DrawInstanced(3, 1, 0, 0);

  D3D12_RESOURCE_BARRIER presentBarrier = {
    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
    .Transition = {
      .pResource = target,
      .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
      .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
      .StateAfter = D3D12_RESOURCE_STATE_PRESENT
    }
  };

  m_CmdList->ResourceBarrier(1, &presentBarrier);

  m_CmdList->Close();

  ID3D12CommandList* cmdLists[] = { m_CmdList.Get() };
  m_CmdQueue->ExecuteCommandLists(static_cast<uint32_t>(std::size(cmdLists)), cmdLists);

  m_SwapChain->Present(1, 0);

  m_PresentFenceValue++;
  m_CmdQueue->Signal(m_PresentFence.Get(), m_PresentFenceValue);
}

void App::CreateDevice() {
  ComPtr<ID3D12Debug1> debug;
  D3D12GetDebugInterface(IID_PPV_ARGS(&debug));

  debug->EnableDebugLayer();
  debug->SetEnableGPUBasedValidation(true);

  CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_Factory));

  static constexpr auto featureLevel = D3D_FEATURE_LEVEL_12_1;

  ComPtr<IDXGIAdapter1> adapter;

  uint32_t adapterIdx = 0;
  while (m_Factory->EnumAdapterByGpuPreference(adapterIdx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
      IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND) {
    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, _uuidof(ID3D12Device), nullptr))) {
      break;
    }

    adapterIdx++;
  }

  D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&m_Device));
}

void App::CreateSwapChainAndCmdList() {
  D3D12_COMMAND_QUEUE_DESC queueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };
  m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CmdQueue));

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
    .Width = m_ScreenWidth,
    .Height = m_ScreenHeight,
    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
    .SampleDesc = {.Count = 1 },
    .BufferUsage = DXGI_USAGE_BACK_BUFFER,
    .BufferCount = kFrameCount,
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
  };

  ComPtr<IDXGISwapChain1> swapChain;
  m_Factory->CreateSwapChainForHwnd(m_CmdQueue.Get(), m_Hwnd, &swapChainDesc, nullptr, nullptr,
      &swapChain);

  swapChain.As(&m_SwapChain);

  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    .NumDescriptors = kFrameCount,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
  };

  m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap));

  m_RtvHandleSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  for (uint8_t i = 0; i < kFrameCount; i++) {
    m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]));

    auto rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += i * m_RtvHandleSize;

    m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);
  }

  m_PresentFenceValue = 0;
  m_Device->CreateFence(m_PresentFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_PresentFence));

  m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAlloc));
  m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAlloc.Get(), nullptr,
      IID_PPV_ARGS(&m_CmdList));

  m_CmdList->Close();
}

void App::CreatePipeline() {
  D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {
    .NumParameters = 0,
    .pParameters = nullptr,
    .NumStaticSamplers = 0,
    .pStaticSamplers = nullptr,
    .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
  };

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

  m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
      IID_PPV_ARGS(&m_RootSig));

  auto vsData = ReadFile("ShaderVS.cso");
  auto psData = ReadFile("ShaderPS.cso");

  D3D12_SHADER_BYTECODE vs = {
    .pShaderBytecode = vsData.data(),
    .BytecodeLength = vsData.size()
  };

  D3D12_SHADER_BYTECODE ps = {
    .pShaderBytecode = psData.data(),
    .BytecodeLength = psData.size()
  };

  D3D12_BLEND_DESC blend = {
    .AlphaToCoverageEnable = false,
    .IndependentBlendEnable = false,
    .RenderTarget = {
      {
        .BlendEnable = false,
        .LogicOpEnable = false,
        .SrcBlend = D3D12_BLEND_ONE,
        .DestBlend = D3D12_BLEND_ZERO,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
      }
    }
  };

  D3D12_RASTERIZER_DESC rasterizer = {
    .FillMode = D3D12_FILL_MODE_SOLID,
    .CullMode = D3D12_CULL_MODE_BACK,
    .FrontCounterClockwise = false,
    .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
    .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    .DepthClipEnable = true,
    .MultisampleEnable = false,
    .AntialiasedLineEnable = false,
    .ForcedSampleCount = 0,
    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
  };

  D3D12_DEPTH_STENCIL_DESC depthStencil = {
    .DepthEnable = false,
    .StencilEnable = false
  };

  D3D12_INPUT_ELEMENT_DESC inputs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0 }
  };

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
    .pRootSignature = m_RootSig.Get(),
    .VS = vs,
    .PS = ps,
    .BlendState = blend,
    .SampleMask = UINT_MAX,
    .RasterizerState = rasterizer,
    .DepthStencilState = depthStencil,
    .InputLayout = {
      .pInputElementDescs = inputs,
      .NumElements = static_cast<UINT>(std::size(inputs))
     },
    .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
    .NumRenderTargets = 1,
    .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
    .SampleDesc = {.Count = 1 }
  };

  m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pipeline));
}

void App::CreateVertexBuffer() {
  constexpr float vertData[] = {
    -0.5f, -0.5f, 0.f, 1.f, 0.f, 0.f, 1.f,
    0.f, 0.5f, 0.f,    0.f, 1.f, 0.f, 1.f,
    0.5f, -0.5f, 0.f,  0.f, 0.f, 1.f, 1.f
  };
  const auto vertDataSize = sizeof(vertData);

  D3D12_HEAP_PROPERTIES heapProps = {
    .Type = D3D12_HEAP_TYPE_UPLOAD,
    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    .CreationNodeMask = 1,
    .VisibleNodeMask = 1
  };

  D3D12_RESOURCE_DESC bufferDesc = {
    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Alignment = 0,
    .Width = vertDataSize,
    .Height = 1,
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .Format = DXGI_FORMAT_UNKNOWN,
    .SampleDesc = {.Count = 1, .Quality = 0 },
    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    .Flags = D3D12_RESOURCE_FLAG_NONE
  };

  m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertBuffer));
  {
    std::byte* mapped = nullptr;

    D3D12_RANGE range = { .Begin = 0, .End = 0 };
    m_VertBuffer->Map(0, &range, reinterpret_cast<void**>(&mapped));

    memcpy(mapped, vertData, vertDataSize);

    m_VertBuffer->Unmap(0, nullptr);
  }

  m_VertBufferView = {
    .BufferLocation = m_VertBuffer->GetGPUVirtualAddress(),
    .SizeInBytes = vertDataSize,
    .StrideInBytes = sizeof(float) * 7
  };
}

std::vector<std::byte> App::ReadFile(std::filesystem::path path) {
  std::vector<std::byte> result;

  std::ifstream strm(path, std::ios::binary | std::ios::ate);
  if (!strm.is_open()) {
    return result;
  }

  auto size = strm.tellg();
  strm.seekg(0, std::ios::beg);

  result.resize(size);

  strm.read(reinterpret_cast<char*>(result.data()), size);

  return result;
}