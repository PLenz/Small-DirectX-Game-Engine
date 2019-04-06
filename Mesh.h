#pragma once
#include <map>
#include "BoundingVolume.h"

class Mesh {
public:
  Mesh(std::wstring objPath, std::wstring pngPath) {
    m_objPath = objPath;
    m_texturePath = pngPath;
  };


  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                     ComPtr<ID3D12CommandAllocator>& commandAllocator);
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                           D3D12_GPU_VIRTUAL_ADDRESS constantBufferUploadHeap);
  void Update(int frameIndex, double delta);
  virtual void Release();
  bool Compare(const Mesh& r);
  int getIListSize();
  BoundingVolume* GetBoundingVolume();


private:
  bool CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                         DWORD* iList, int indexBufferSize);
  bool CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                          Vertex* vList, int vertexBufferSize);
  bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);

  ComPtr<ID3D12Resource> m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  ComPtr<ID3D12Resource> m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
  int m_iListSize;

  ComPtr<ID3D12Resource> m_textureBuffer; // the resource heap containing our texture
  ComPtr<ID3D12Resource> m_textureBufferUploadHeap;

  ComPtr<ID3D12DescriptorHeap> m_shaderResourceViewDescriptorHeap;

  std::wstring m_objPath;
  std::wstring m_texturePath;
  BoundingVolume* m_boundingVolume;

  BYTE* m_imageData;

  bool loaded = false;
};
