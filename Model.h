#pragma once
#include "Mesh.h"
#include "MeshCache.h"

class Model {
public:
  Model() {};


  Model(std::wstring objPath, std::wstring texturePath, XMFLOAT3 position, XMFLOAT4 rotation, bool solid);
  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                     ComPtr<ID3D12CommandAllocator>& commandAllocator);
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT8* constantBufferGPUAddress,
                           D3D12_GPU_VIRTUAL_ADDRESS constantBufferUploadHeap);
  void Update(float delta);
  void Release();
  void Unload();
  bool Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution);
  bool CheckCull(BoundingVolume*& frustum);
  XMFLOAT3 getPosition();
  void setPosition(XMFLOAT3 pos);
  Mesh* GetMesh();
  void SetCull(bool cullModel, ComPtr<ID3D12GraphicsCommandList>& commandList,
               ComPtr<ID3D12CommandAllocator>& commandAllocator);
  boolean m_reload = false;

private:
  boolean m_isCulled;
  Mesh* m_mesh;
  ConstantBuffer m_constantBuffer;
  XMFLOAT3 m_position;
  XMFLOAT4 m_rotation;
  bool m_solid;
  std::wstring m_objPath;
  std::wstring m_texturePath;
};
