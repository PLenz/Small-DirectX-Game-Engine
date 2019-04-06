#pragma once
#include "BoundingVolume.h"

class Renderer {
public:
  Renderer() {};


  ~Renderer() {};


  virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int, int);
  virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                              ComPtr<ID3D12CommandAllocator>& command_allocator, int, int);

  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                                     ComPtr<ID3D12CommandAllocator>& command_allocator,
                                     CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle, ComPtr<ID3D12Resource>& render_target,
                                     int frame_count);
  virtual void Update(int frame_index, double delta);
  virtual void Release();
  virtual bool Intersects(BoundingVolume& camera, XMFLOAT3& resolution);
};
