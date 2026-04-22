#include "ShaderShared.h"

PSInput VSMain(float3 position: POSITION, float2 uv: TEXCOORD) {
  float4x4 rotation = float4x4(
    0.7, 0.0, 0.7, 0.0,
    0.0, 1.0, 0.0, 0.0,
    -0.7, 0.0, 0.7, 0.0,
    0.0, 0.0, 0.0, 1.0
  );
  
  float4x4 translate = float4x4(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 5.0, 1.0
  );
  
  float4x4 view = mul(rotation, translate);
  
  float aspect = 1024.0 / 768.0;
  
  float horizontalFov = 1.57;
  float verticalFov = horizontalFov / aspect;
  
  float nearPlane = 0.1;
  float farPlane = 10.0;
  
  float w = 1.0 / tan(horizontalFov * 0.5);
  float h = 1.0 / tan(verticalFov * 0.5);
 
  float zRatio = farPlane / (farPlane - nearPlane);
  
  float4x4 projection = float4x4(
    w, 0.0, 0.0, 0.0,
    0.0, h, 0.0, 0.0,
    0.0, 0.0, zRatio, 1.0,
    0.0, 0.0, -zRatio * nearPlane, 0.0
  );
  
  float4x4 mvp = mul(view, projection);
    
  PSInput result;
  result.position = mul(float4(position, 1.0), mvp);
  result.uv = uv;
    
  return result;
}