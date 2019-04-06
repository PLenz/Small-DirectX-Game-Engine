#pragma once
#include "Mesh.h"
#include "MeshCache.h"

class Model {
public:
  Model() {};


  Model(std::wstring obj_path, std::wstring texture_path, XMFLOAT3 position, XMFLOAT4 rotation, bool solid);
  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                     ComPtr<ID3D12CommandAllocator>& command_allocator);
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list, UINT8* constant_buffer_gpu_address,
                           D3D12_GPU_VIRTUAL_ADDRESS constant_buffer_upload_heap);
  void Update(float delta);
  void Release();
  void Unload();
  bool Intersects(BoundingVolume& camera, XMFLOAT3& resolution);
  bool CheckCull(BoundingVolume*& frustum);
  XMFLOAT3 getPosition();
  void setPosition(XMFLOAT3 pos);
  Mesh* GetMesh();
  void SetCull(bool cull, ComPtr<ID3D12GraphicsCommandList>& command_list,
               ComPtr<ID3D12CommandAllocator>& command_allocator);
  boolean m_reload_ = false;

private:
  boolean m_is_culled_;
  Mesh* m_mesh_;
  ConstantBuffer m_constant_buffer_;
  XMFLOAT3 m_position_;
  XMFLOAT4 m_rotation_;
  bool m_solid_;
  std::wstring m_obj_path_;
  std::wstring m_texture_path_;
};
