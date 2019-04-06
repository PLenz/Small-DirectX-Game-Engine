#pragma once
#include "DepthQuadRenderer.h"
#include "TextureLoader.h"

class ImageRenderer :
	public DepthQuadRenderer
{
public:
	ImageRenderer()
	{
	};

	~ImageRenderer()
	{
	};

	virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height);
	virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
	                           ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height);

	virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
	                                 ComPtr<ID3D12CommandAllocator>& commandAllocator,
	                                 CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget,
	                                 int frameIndex);
	virtual void Update(int frameIndex, double delta);

	virtual void Release();

protected:
	ComPtr<ID3D12Resource> m_textureBuffer; // the resource heap containing our texture
	ComPtr<ID3D12Resource> m_textureBufferUploadHeap;

	BYTE* m_logoData;

	virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);
	virtual bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
};
