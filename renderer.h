#pragma once
#include "BoundingVolume.h"

class Renderer {
public:
  Renderer() {};


  ~Renderer() {};


  virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int, int);
  virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                             ComPtr<ID3D12CommandAllocator>& commandAllocator, int, int);

  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                                   ComPtr<ID3D12CommandAllocator>& commandAllocator,
                                   CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget,
                                   int frameCount);
  virtual void Update(int frameIndex, double delta);
  virtual void Release();
  virtual bool Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution);
};
