#include "ShaderShared.h"

Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

float4 PSMain(PSInput input): SV_TARGET {
  return g_Texture.Sample(g_Sampler, input.uv);
}