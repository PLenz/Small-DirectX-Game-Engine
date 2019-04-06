#pragma once
#include <map>
#include "BoundingVolume.h"

class Mesh {
public:
  Mesh(std::wstring obj_path, std::wstring png_path) {
    m_obj_path_ = obj_path;
    m_texture_path_ = png_path;
  };


  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                     ComPtr<ID3D12CommandAllocator>& command_allocator);
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                           D3D12_GPU_VIRTUAL_ADDRESS constant_buffer_upload_heap);
  void Update(int frame_index, double delta);
  virtual void Release();
  bool Compare(const Mesh& mesh);
  int getIListSize();
  BoundingVolume* GetBoundingVolume();


private:
  bool CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                         DWORD* i_list, int index_buffer_size);
  bool CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                          Vertex* v_list, int vertex_buffer_size);
  bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list);

  ComPtr<ID3D12Resource> m_vertex_buffer_;
  D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_;

  ComPtr<ID3D12Resource> m_index_buffer_;
  D3D12_INDEX_BUFFER_VIEW m_index_buffer_view_;
  int m_i_list_size_;

  ComPtr<ID3D12Resource> m_texture_buffer_; // the resource heap containing our texture
  ComPtr<ID3D12Resource> m_texture_buffer_upload_heap_;

  ComPtr<ID3D12DescriptorHeap> m_shader_resource_view_descriptor_heap_;

  std::wstring m_obj_path_;
  std::wstring m_texture_path_;
  BoundingVolume* p_bounding_volume_;

  BYTE* p_data_;

  bool m_loaded_ = false;
};
