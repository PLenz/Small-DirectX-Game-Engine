#pragma once
#include "Renderer.h"

class DynTriangleRenderer :
  public Renderer {
public:
  DynTriangleRenderer(int vertexCount);


  ~DynTriangleRenderer() {};


  bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height) override;
  bool LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                      ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                             ComPtr<ID3D12Resource>& render_target, int frame_index) override;
  void Release() override;

protected:
  CD3DX12_VIEWPORT m_viewport_;
  CD3DX12_RECT m_scissor_rect_;

  ComPtr<ID3D12RootSignature> m_root_signature_;
  ComPtr<ID3D12PipelineState> m_pipeline_state_;

  ComPtr<ID3D12Resource> m_vertex_buffer_;
  D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_;
  int m_vertex_count_;
  int m_triangle_count_;

  virtual bool CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                                  Vertex* vertex_list, int vertex_buffer_size);
  virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  virtual bool CompileShaders(ComPtr<ID3DBlob>& vertex_shader, LPCSTR entry_point_vertex_shader,
                              ComPtr<ID3D10Blob>& pixel_shader, LPCSTR entry_point_pixel_shader);
};
