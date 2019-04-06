#pragma once
#include "DepthQuadRenderer.h"
#include "TextureLoader.h"

class ImageRenderer :
  public DepthQuadRenderer {
public:
  ImageRenderer() { };


  ~ImageRenderer() { };


  bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height) override;
  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                      ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) override;

  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle, ComPtr<ID3D12Resource>& render_target,
                             int frame_index) override;
  void Update(int frame_index, double delta) override;

  void Release() override;

protected:
  ComPtr<ID3D12Resource> m_texture_buffer_; // the resource heap containing our texture
  ComPtr<ID3D12Resource> m_texture_buffer_upload_heap_;

  BYTE* p_data_;

  bool CreateRootSignature(ComPtr<ID3D12Device>& device) override;
  virtual bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
};
