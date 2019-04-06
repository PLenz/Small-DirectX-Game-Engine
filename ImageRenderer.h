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
                     ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height) override;

  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                           ComPtr<ID3D12CommandAllocator>& commandAllocator,
                           CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget,
                           int frameIndex) override;
  void Update(int frameIndex, double delta) override;

  void Release() override;

protected:
  ComPtr<ID3D12Resource> m_textureBuffer; // the resource heap containing our texture
  ComPtr<ID3D12Resource> m_textureBufferUploadHeap;

  BYTE* m_logoData;

  bool CreateRootSignature(ComPtr<ID3D12Device>& device) override;
  virtual bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
};
