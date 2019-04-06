#pragma once
#include "renderer.h"
#include "model.h"
#include <vector>

class WorldRenderer :
  public Renderer {
public:
  WorldRenderer() { };


  ~WorldRenderer() {};


  bool CreatePipelineState(ComPtr<ID3D12Device>&, int, int) override;
  bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                     ComPtr<ID3D12CommandAllocator>& commandAllocator, int, int) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                           ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                           ComPtr<ID3D12Resource>& renderTarget, int frameCount) override;
  void Update(int frameIndex, double delta) override;
  void Release() override;
  bool Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution) override;

protected:
  std::vector<Model> m_models;

  CD3DX12_VIEWPORT m_viewport;
  CD3DX12_RECT m_scissorRect;

  ComPtr<ID3D12RootSignature> m_rootSignature;
  ComPtr<ID3D12PipelineState> m_pipelineState;

  ComPtr<ID3D12Resource> m_depthStencilBuffer;
  ComPtr<ID3D12DescriptorHeap> m_depthStencilDescriptorHeap;
  ComPtr<ID3D12DescriptorHeap> m_constantBufferDescriptorHeap[DXGE_FRAME_COUNT];
  ComPtr<ID3D12Resource> m_constantBufferUploadHeap[DXGE_FRAME_COUNT];
  UINT8* m_constantBufferGPUAddress[DXGE_FRAME_COUNT];
  int numElementsToDraw;
  int numMeshesToDraw;
  std::vector<XMFLOAT3> culledModels;

  bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  bool CompileShaders(ComPtr<ID3DBlob>& vertexShader, LPCSTR entryPointVertexShader, ComPtr<ID3D10Blob>& pixelShader,
                      LPCSTR entryPointPixelShader);
  bool CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, int width,
                                int height);
  bool CreateConstantBuffer(ComPtr<ID3D12Device>& device);
  bool cullAABB(std::vector<Model*>& culledModels, ComPtr<ID3D12GraphicsCommandList>& commandList,
                ComPtr<ID3D12CommandAllocator>& commandAllocator);
};
