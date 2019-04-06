#pragma once
#include "QuadRenderer.h"
class DepthQuadRenderer :
    public QuadRenderer
{
public:
    DepthQuadRenderer() {};
    ~DepthQuadRenderer() {};

    virtual bool CreatePipelineState(ComPtr<ID3D12Device> &device, int width, int height);
    virtual bool LoadResources(ComPtr<ID3D12Device> &device, ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, int width, int height);
	virtual void Update(int frameIndex, double delta);
    virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE &rtvHandle, ComPtr<ID3D12Resource> &renderTarget, int frameIndex);

    virtual void Release();

protected:
    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_depthStencilDescriptorHeap;
	ConstantBuffer m_constantBuffer;

    ComPtr<ID3D12DescriptorHeap> m_mainDescriptorHeap[DXGE_FRAME_COUNT];
    ComPtr<ID3D12Resource> m_constantBufferUploadHeap[DXGE_FRAME_COUNT];
    UINT8* m_constantBufferGPUAddress[DXGE_FRAME_COUNT];
	float angle;

    virtual bool CreateRootSignature(ComPtr<ID3D12Device> &device);
	virtual bool CreateConstantBuffer(ComPtr<ID3D12Device> &device);
    virtual bool CreateDepthStencilBuffer(ComPtr<ID3D12Device> &device, ComPtr<ID3D12GraphicsCommandList> &commandList, int width, int height);
};