#pragma once
#include "TriangleRenderer.h"
class QuadRenderer :
    public TriangleRenderer
{
public:
    QuadRenderer() {};
    ~QuadRenderer() {};

    virtual bool LoadResources(ComPtr<ID3D12Device> &device, ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, int width, int height);
    virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE &rtvHandle, ComPtr<ID3D12Resource> &renderTarget, int frameIndex);
    virtual void Release();

protected:
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	int iListSize;

    virtual bool CreateIndexBuffer(ComPtr<ID3D12Device> &device, ComPtr<ID3D12GraphicsCommandList> &commandList, DWORD* iList, int indexBufferSize);
};