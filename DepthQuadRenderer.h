#pragma once
#include "QuadRenderer.h"

class DepthQuadRenderer :
  public QuadRenderer {
public:
  DepthQuadRenderer() {};


  ~DepthQuadRenderer() {};


  bool CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height) override;
  bool LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                      ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) override;
  void Update(int frame_index, double delta) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                             ComPtr<ID3D12Resource>& render_target, int frame_index) override;

  void Release() override;

protected:
  ComPtr<ID3D12Resource> m_depth_stencil_buffer_;
  ComPtr<ID3D12DescriptorHeap> m_depth_stencil_descriptor_heap_;
  ConstantBuffer m_constant_buffer_;

  ComPtr<ID3D12DescriptorHeap> m_main_descriptor_heap_[DXGE_FRAME_COUNT];
  ComPtr<ID3D12Resource> m_constant_buffer_upload_heap_[DXGE_FRAME_COUNT];
  UINT8* p_constant_buffer_gpu_address_[DXGE_FRAME_COUNT];
  float m_angle_;

  bool CreateRootSignature(ComPtr<ID3D12Device>& device) override;
  virtual bool CreateConstantBuffer(ComPtr<ID3D12Device>& device);
  virtual bool CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device,
                                           ComPtr<ID3D12GraphicsCommandList>& command_list,
                                           int width, int height);
};
