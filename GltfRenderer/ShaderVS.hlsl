#include "ShaderShared.h"

cbuffer ConstantBuffer : register(b0) {
  row_major float4x4 g_MvpMat;
};

PSInput VSMain(float3 position: POSITION, float2 uv: TEXCOORD) {
  PSInput result;
  result.position = mul(float4(position, 1.0), g_MvpMat);
  result.uv = uv;
    
  return result;
}