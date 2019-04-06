#include "stdafx.h"
#include "Renderer.h"


bool Renderer::CreatePipelineState(ComPtr<ID3D12Device>&, int, int) {
  return true;
}


bool Renderer::LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&,
                              ComPtr<ID3D12CommandAllocator>& command_allocator, int, int) {
  return true;
}


bool Renderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                                     ComPtr<ID3D12CommandAllocator>& command_allocator,
                                     CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle, ComPtr<ID3D12Resource>& render_target,
                                     int frameIndex) {
  // Reset the command Allocator
  HRESULT hr = command_allocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error command_allocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = command_list->Reset(command_allocator.Get(), nullptr);
  if (FAILED(hr)) {
    Log::Error("Error command_list->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  /*	Notifies the driver that it needs to synchronize multiple accesses to resources.
    Set Resource Barrier for render target
    NumBarriers: The number of submitted barrier descriptions.
    pBarriers: Pointer to an array of barrier descriptions.

    Before: present
    After: render target
  */
  command_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET));

  /* Sets CPU descriptor handles for the render targets and depth stencil.
    NumRenderTargetDescriptors: The number of entries in the pRenderTargetDescriptors array.
    pRenderTargetDescriptors: Specifies an array of D3D12_CPU_DESCRIPTOR_HANDLE structures that describe the CPU descriptor handles that represents the start of the heap of render target descriptors.
    RTsSingleHandleToDescriptorRange: 
      true: the handle passed in is the pointer to a contiguous range of NumRenderTargetDescriptors descriptors.
      false: the handle is the first of an array of NumRenderTargetDescriptors handles
    pDepthStencilDescriptor: A pointer to a D3D12_CPU_DESCRIPTOR_HANDLE structure that describes the CPU descriptor handle that represents the start of the heap that holds the depth stencil descriptor.
  */
  command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

  // Set THM colors
  const float clearColor[] = {0.29f, 0.36f, 0.4f, 1.0f};

  /* Sets all the elements in a render target to one value.
    RenderTargetView: Specifies a D3D12_CPU_DESCRIPTOR_HANDLE structure that describes the CPU descriptor handle that represents the start of the heap for the render target to be cleared.
    ColorRGBA: A 4-component array that represents the color to fill the render target with.
    NumRects: The number of rectangles in the array that the pRects parameter specifies.
    pRects: An array of D3D12_RECT structures for the rectangles in the resource view to clear. If NULL, ClearRenderTargetView clears the entire resource view.
  */
  command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

  /*	Notifies the driver that it needs to synchronize multiple accesses to resources.
    Set Resource Barrier for render target
    NumBarriers: The number of submitted barrier descriptions.
    pBarriers: Pointer to an array of barrier descriptions.

    Before: render target
    After: present
  */
  command_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT));

  // close command list
  hr = command_list->Close();
  if (FAILED(hr)) {
    Log::Error("Error command_list->Close() - ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


void Renderer::Release() {}


bool Renderer::Intersects(BoundingVolume& camera, XMFLOAT3& resolution) {
  return false;
}


void Renderer::Update(int frame_index, double delta) { }
