#include "stdafx.h"
#include "renderer.h"


bool Renderer::CreatePipelineState(ComPtr<ID3D12Device>&, int, int) {
  return true;
}


bool Renderer::LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                             ComPtr<ID3D12CommandAllocator>& commandAllocator, int, int) {
  return true;
}


bool Renderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                                   ComPtr<ID3D12CommandAllocator>& commandAllocator,
                                   CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget,
                                   int frameIndex) {
  // Reset the command Allocator
  HRESULT hr = commandAllocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error commandAllocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = commandList->Reset(commandAllocator.Get(), nullptr);
  if (FAILED(hr)) {
    Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  /*	Notifies the driver that it needs to synchronize multiple accesses to resources.
    Set Resource Barrier for render target
    NumBarriers: The number of submitted barrier descriptions.
    pBarriers: Pointer to an array of barrier descriptions.

    Before: present
    After: render target
  */
  commandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET));

  /* Sets CPU descriptor handles for the render targets and depth stencil.
    NumRenderTargetDescriptors: The number of entries in the pRenderTargetDescriptors array.
    pRenderTargetDescriptors: Specifies an array of D3D12_CPU_DESCRIPTOR_HANDLE structures that describe the CPU descriptor handles that represents the start of the heap of render target descriptors.
    RTsSingleHandleToDescriptorRange: 
      true: the handle passed in is the pointer to a contiguous range of NumRenderTargetDescriptors descriptors.
      false: the handle is the first of an array of NumRenderTargetDescriptors handles
    pDepthStencilDescriptor: A pointer to a D3D12_CPU_DESCRIPTOR_HANDLE structure that describes the CPU descriptor handle that represents the start of the heap that holds the depth stencil descriptor.
  */
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  // Set THM colors
  const float clearColor[] = {0.29f, 0.36f, 0.4f, 1.0f};

  /* Sets all the elements in a render target to one value.
    RenderTargetView: Specifies a D3D12_CPU_DESCRIPTOR_HANDLE structure that describes the CPU descriptor handle that represents the start of the heap for the render target to be cleared.
    ColorRGBA: A 4-component array that represents the color to fill the render target with.
    NumRects: The number of rectangles in the array that the pRects parameter specifies.
    pRects: An array of D3D12_RECT structures for the rectangles in the resource view to clear. If NULL, ClearRenderTargetView clears the entire resource view.
  */
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  /*	Notifies the driver that it needs to synchronize multiple accesses to resources.
    Set Resource Barrier for render target
    NumBarriers: The number of submitted barrier descriptions.
    pBarriers: Pointer to an array of barrier descriptions.

    Before: render target
    After: present
  */
  commandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT));

  // close command list
  hr = commandList->Close();
  if (FAILED(hr)) {
    Log::Error("Error commandList->Close() - ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


void Renderer::Release() {}


bool Renderer::Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution) {
  return false;
}


void Renderer::Update(int frameIndex, double delta) { }
