#pragma once
#include "TriangleRenderer.h"

class QuadRenderer :
  public TriangleRenderer {
public:
  QuadRenderer() {};


  ~QuadRenderer() {};


  bool LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                      ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                             ComPtr<ID3D12Resource>& render_target, int frame_index) override;
  void Release() override;

protected:
  ComPtr<ID3D12Resource> m_index_buffer_;
  D3D12_INDEX_BUFFER_VIEW m_index_buffer_view_;
  int i_list_size_;

  virtual bool CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                                 DWORD* i_list, int index_buffer_size);
};
