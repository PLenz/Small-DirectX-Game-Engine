#pragma once
#include "Renderer.h"
#include "model.h"
#include <vector>

class WorldRenderer :
  public Renderer {
public:
  WorldRenderer() { };


  ~WorldRenderer() {};


  bool CreatePipelineState(ComPtr<ID3D12Device>&, int, int) override;
  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                      ComPtr<ID3D12CommandAllocator>& command_allocator, int, int) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                             ComPtr<ID3D12Resource>& render_target, int frame_count) override;
  void Update(int frame_index, double delta) override;
  void Release() override;
  bool Intersects(BoundingVolume& camera, XMFLOAT3& resolution) override;

protected:
  std::vector<Model> m_models_;

  CD3DX12_VIEWPORT m_viewport_;
  CD3DX12_RECT m_scissor_rect_;

  ComPtr<ID3D12RootSignature> m_root_signature_;
  ComPtr<ID3D12PipelineState> m_pipeline_state_;

  ComPtr<ID3D12Resource> m_depth_stencil_buffer_;
  ComPtr<ID3D12DescriptorHeap> m_depth_stencil_descriptor_heap_;
  ComPtr<ID3D12DescriptorHeap> m_constant_buffer_descriptor_heap_[DXGE_FRAME_COUNT];
  ComPtr<ID3D12Resource> m_constant_buffer_upload_heap_[DXGE_FRAME_COUNT];
  UINT8* p_constant_buffer_gpu_address_[DXGE_FRAME_COUNT];
  int m_num_elements_to_draw_;
  int m_num_meshes_to_draw_;
  std::vector<XMFLOAT3> m_culled_models_;

  bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  bool CompileShaders(ComPtr<ID3DBlob>& vertex_shader, LPCSTR entry_point_vertex_shader, ComPtr<ID3D10Blob>& pixel_shader,
                      LPCSTR entry_point_pixel_shader);
  bool CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list, int width,
                                int height);
  bool CreateConstantBuffer(ComPtr<ID3D12Device>& device);
  bool FrustumCulling(std::vector<Model*>& culled_models, ComPtr<ID3D12GraphicsCommandList>& command_list,
                ComPtr<ID3D12CommandAllocator>& command_allocator);
};
