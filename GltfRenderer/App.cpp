#include "App.h"

#include <fstream>

using Microsoft::WRL::ComPtr;

bool App::Init(HWND hwnd) {
  if (auto res = LoadGltf("D:/glTF-Sample-Assets/Models/Cube/glTF/Cube.gltf")) {
    m_Model = *res;
  } else {
    return false;
  }

  m_Hwnd = hwnd;

  RECT wndRect = {};
  GetClientRect(m_Hwnd, &wndRect);

  m_ScreenWidth = wndRect.right;
  m_ScreenHeight = wndRect.bottom;

  CreateDevice();

  CreateSwapChainAndCmdList();

  CreatePipeline();

  CreateDescriptorHeaps();

  CreateIndexBuffer();
  CreateVertexBuffers();

  CreateTexture();

  return true;
}

void App::Render() {
  if (m_Fence->GetCompletedValue() < m_FenceValue) {
    return;
  }

  m_FrameIdx = m_SwapChain->GetCurrentBackBufferIndex();

  m_CmdAlloc->Reset();
  m_CmdList->Reset(m_CmdAlloc.Get(), m_Pipeline.Get());

  m_CmdList->SetGraphicsRootSignature(m_RootSig.Get());

  ID3D12DescriptorHeap* heaps[] = { m_SrvHeap.Get() };
  m_CmdList->SetDescriptorHeaps(std::size(heaps), heaps);

  m_CmdList->SetGraphicsRootDescriptorTable(0, m_SrvHeap->GetGPUDescriptorHandleForHeapStart());

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
  m_CmdList->IASetIndexBuffer(&m_IdxBufferView);

  D3D12_VERTEX_BUFFER_VIEW vertBufferViews[] = { m_PosBufferView, m_UvBufferView };
  m_CmdList->IASetVertexBuffers(0, std::size(vertBufferViews), vertBufferViews);

  auto vertCount = m_Model.IdxBuffer.Data.size();
  m_CmdList->DrawInstanced(static_cast<uint32_t>(vertCount), 1, 0, 0);

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

  m_FenceValue++;
  m_CmdQueue->Signal(m_Fence.Get(), m_FenceValue);
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

  m_FenceValue = 0;
  m_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));

  m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAlloc));
  m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAlloc.Get(), nullptr,
      IID_PPV_ARGS(&m_CmdList));

  m_CmdList->Close();
}

void App::CreatePipeline() {
  D3D12_DESCRIPTOR_RANGE1 range = {
    .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    .NumDescriptors = 1,
    .BaseShaderRegister = 0,
    .RegisterSpace = 0,
    .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
  };

  D3D12_ROOT_PARAMETER1 rootParam = {
    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
    .DescriptorTable = {
      .NumDescriptorRanges = 1,
      .pDescriptorRanges = &range
    },
    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
  };

  D3D12_STATIC_SAMPLER_DESC sampler = {
    .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
    .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    .MipLODBias = 0,
    .MaxAnisotropy = 0,
    .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
    .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
    .MinLOD = 0.f,
    .MaxLOD = D3D12_FLOAT32_MAX,
    .ShaderRegister = 0,
    .RegisterSpace = 0,
    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
  };

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {
    .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
    .Desc_1_1 = {
      .NumParameters = 1,
      .pParameters = &rootParam,
      .NumStaticSamplers = 1,
      .pStaticSamplers = &sampler,
      .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    }
  };

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, &error);

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

  //D3D12_DEPTH_STENCIL_DESC depthStencil = {
  //  .DepthEnable = true,
  //  .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
  //  .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
  //  .StencilEnable = false
  //};

  D3D12_INPUT_ELEMENT_DESC inputs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
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

void App::CreateIndexBuffer() {
  auto size = m_Model.IdxBuffer.Data.size() * sizeof(uint16_t);

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
    .Width = size,
    .Height = 1,
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .Format = DXGI_FORMAT_UNKNOWN,
    .SampleDesc = {.Count = 1, .Quality = 0 },
    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    .Flags = D3D12_RESOURCE_FLAG_NONE
  };

  m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IdxBuffer));

  std::byte* mapped = nullptr;
  m_IdxBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

  memcpy(mapped, m_Model.IdxBuffer.Data.data(), size);

  m_IdxBuffer->Unmap(0, nullptr);

  m_IdxBufferView = {
    .BufferLocation = m_IdxBuffer->GetGPUVirtualAddress(),
    .SizeInBytes = static_cast<uint32_t>(size),
    .Format = DXGI_FORMAT_R16_UINT
  };
}

void App::CreateDescriptorHeaps() {
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = 1,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
  };

  m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SrvHeap));
}

void App::CreateVertexBuffers() {
  {
    uint32_t stride = m_Model.PosBuffer.NumComponents * sizeof(float);
    uint32_t size = static_cast<uint32_t>(m_Model.PosBuffer.Data.size()) * stride;

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
      .Width = size,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0 },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_PosBuffer));

    std::byte* mapped = nullptr;
    m_PosBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    memcpy(mapped, m_Model.PosBuffer.Data.data(), size);

    m_PosBuffer->Unmap(0, nullptr);

    m_PosBufferView = {
      .BufferLocation = m_PosBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = size,
      .StrideInBytes = stride
    };
  }
  
  {
    uint32_t stride = m_Model.UvBuffer.NumComponents * sizeof(float);
    uint32_t size = static_cast<uint32_t>(m_Model.UvBuffer.Data.size()) * stride;

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
      .Width = size,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0 },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UvBuffer));

    std::byte* mapped = nullptr;
    m_UvBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    memcpy(mapped, m_Model.UvBuffer.Data.data(), size);

    m_UvBuffer->Unmap(0, nullptr);

    m_UvBufferView = {
      .BufferLocation = m_UvBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = size,
      .StrideInBytes = stride
    };
  }
}

void App::CreateTexture() {
  const auto& img = m_Model.Img;

  D3D12_RESOURCE_DESC textureDesc = {
    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    .Alignment = 0,
    .Width = static_cast<uint64_t>(img.Width),
    .Height = static_cast<uint32_t>(img.Height),
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
    .SampleDesc = {.Count = 1, .Quality = 0 },
    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
    .Flags = D3D12_RESOURCE_FLAG_NONE
  };

  {
    D3D12_HEAP_PROPERTIES heapProps = {
      .Type = D3D12_HEAP_TYPE_DEFAULT,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
      .CreationNodeMask = 1,
      .VisibleNodeMask = 1
    };

    m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_Texture));
  }

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;

  uint32_t numRows = 0;
  uint64_t rowSize = 0;

  uint64_t requiredSize = 0;

  m_Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &layout, &numRows, &rowSize, 
      &requiredSize);

  {
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
      .Width = requiredSize,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0 },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadBuffer));
  }

  std::byte* mapped = nullptr;
  m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

  auto* dst = mapped + layout.Offset;
  int dstRowPitch = layout.Footprint.RowPitch;

  const auto* src = img.Data.data();
  int srcRowPitch = img.Width * img.NumChannels;

  for (uint32_t y = 0; y < numRows; y++) {
    memcpy(dst + dstRowPitch * y, src + srcRowPitch * y, rowSize);
  }

  m_UploadBuffer->Unmap(0, nullptr);

  m_CmdAlloc->Reset();
  m_CmdList->Reset(m_CmdAlloc.Get(), nullptr);

  D3D12_TEXTURE_COPY_LOCATION copyDst = {
    .pResource = m_Texture.Get(),
    .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = 0
  };

  D3D12_TEXTURE_COPY_LOCATION copySrc = {
    .pResource = m_UploadBuffer.Get(),
    .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
    .PlacedFootprint = layout
  };

  m_CmdList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);

  D3D12_RESOURCE_BARRIER barrier = {
    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
    .Transition = {
      .pResource = m_Texture.Get(),
      .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
      .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
      .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    }
  };

  m_CmdList->ResourceBarrier(1, &barrier);

  m_CmdList->Close();

  ID3D12CommandList* cmdLists[] = { m_CmdList.Get() };
  m_CmdQueue->ExecuteCommandLists(static_cast<uint32_t>(std::size(cmdLists)), cmdLists);

  m_FenceValue++;
  m_CmdQueue->Signal(m_Fence.Get(), m_FenceValue);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
    .Format = textureDesc.Format,
    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    .Texture2D = {
      .MipLevels = 1
    }
  };

  m_Device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, 
      m_SrvHeap->GetCPUDescriptorHandleForHeapStart());
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