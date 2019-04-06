#pragma once
#include "Renderer.h"

class TriangleRenderer :
  public Renderer {
public:
  TriangleRenderer() {};


  ~TriangleRenderer() {};


  bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height) override;
  bool LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                     ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height) override;
  bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                           ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                           ComPtr<ID3D12Resource>& renderTarget, int frameIndex) override;
  void Release() override;

protected:
  CD3DX12_VIEWPORT m_viewport;
  CD3DX12_RECT m_scissorRect;

  ComPtr<ID3D12RootSignature> m_rootSignature;
  ComPtr<ID3D12PipelineState> m_pipelineState;

  ComPtr<ID3D12Resource> m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  virtual bool CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                                  Vertex* vList, int vertexBufferSize);
  virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  virtual bool CompileShaders(ComPtr<ID3DBlob>& vertexShader, LPCSTR entryPointVertexShader,
                              ComPtr<ID3D10Blob>& pixelShader, LPCSTR entryPointPixelShader);
};
