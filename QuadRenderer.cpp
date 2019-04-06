#include "stdafx.h"
#include "QuadRenderer.h"


bool QuadRenderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                                  ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) {
  HRESULT hr;
  // Wird einmalig beim Start aufgerufen. Implementierung gemäß Vorlesung
  hr = command_allocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error command_allocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = command_list->Reset(command_allocator.Get(), m_pipeline_state_.Get());
  if (FAILED(hr)) {
    Log::Error("Error command_list->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  // create triangle vertices
  Vertex triangleVertices[] =
  {
    {{-0.25f, +0.25f, 0.0f}, THM_GREEN}, // top left
    {{+0.25f, +0.25f, 0.0f}, THM_GREEN}, // top right
    {{+0.25f, -0.25f, 0.0f}, THM_GREEN}, // bottom right
    {{-0.25f, -0.25f, 0.0f}, THM_GREEN}, // bottom left
  };

  // a quad (2 triangles)
  DWORD iList[] = {
    0, 1, 2, // first triangle
    2, 3, 0 // second triangle
  };

  i_list_size_ = _countof(iList);

  if (!this->CreateIndexBuffer(m_device, command_list, iList, sizeof(iList))) {
    Log::Error("Error create index buffer -ERROR:");
    return false;
  }

  if (!this->CreateVertexBuffer(m_device, command_list, triangleVertices, sizeof(triangleVertices))) {
    Log::Error("Error create vertex buffer -ERROR:");
    return false;
  }

  hr = command_list->Close();
  if (FAILED(hr)) {
    Log::Error("Error close command list -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool QuadRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                                         ComPtr<ID3D12CommandAllocator>& command_allocator,
                                         CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle, ComPtr<ID3D12Resource>& render_target,
                                         int frame_index) {
  // Wird mit jedem Frame aufgerufen.
  // we can only reset an allocator once the gpu is done with it
  // resetting an allocator frees the memory that the command list was stored in
  HRESULT hr = command_allocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error command_allocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = command_list->Reset(command_allocator.Get(), m_pipeline_state_.Get());
  if (FAILED(hr)) {
    Log::Error("Error command_list->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  command_list->SetGraphicsRootSignature(m_root_signature_.Get()); // set the root signature
  command_list->RSSetViewports(1, &m_viewport_); // set the viewports
  command_list->RSSetScissorRects(1, &m_scissor_rect_); // set the scissor rects
  // here we start recording commands into the command_list (which all the commands will be stored in the command_allocator)

  // transition the "frame_index" render target from the present state to the render target state so the command list draws to it starting from here
  command_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET));

  // set the render target for the output merger stage (the output of the pipeline)
  command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

  // Clear the render target by using the ClearRenderTargetView command
  const float clearColor[] = THM_GREY;
  command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

  // draw triangle
  command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
  command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view_); // set the vertex buffer (using the vertex buffer view)
  command_list->IASetIndexBuffer(&m_index_buffer_view_);
  command_list->DrawIndexedInstanced(i_list_size_, 1, 0, 0, 0); // finally draw 3 vertices (draw the triangle)

  // transition the "frame_index" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
  // warning if present is called on the render target when it's not in the present state
  command_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT));

  hr = command_list->Close();
  if (FAILED(hr)) {
    Log::Error("Error close command list -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool QuadRenderer::CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                                     DWORD* i_list, int index_buffer_size) {
  // create default heap to hold index buffer
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(index_buffer_size), // resource description for a buffer
    D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
    nullptr, // optimized clear value must be null for this type of resource
    IID_PPV_ARGS(&m_index_buffer_));

  // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
  m_index_buffer_->SetName(L"Index Buffer Resource Heap");

  // create upload heap to upload index buffer
  ID3D12Resource* iBufferUploadHeap;
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(index_buffer_size), // resource description for a buffer
    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
    nullptr,
    IID_PPV_ARGS(&iBufferUploadHeap));
  iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

  // store index buffer in upload heap
  D3D12_SUBRESOURCE_DATA indexData = {};
  indexData.pData = reinterpret_cast<BYTE*>(i_list); // pointer to our index array
  indexData.RowPitch = index_buffer_size; // size of all our index buffer
  indexData.SlicePitch = index_buffer_size; // also the size of our index buffer

  // we are now creating a command with the command list to copy the data from
  // the upload heap to the default heap
  UpdateSubresources(command_list.Get(), m_index_buffer_.Get(), iBufferUploadHeap, 0, 0, 1, &indexData);

  // transition the vertex buffer data from copy destination state to vertex buffer state
  command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_index_buffer_.Get(),
                                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  // create a index buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
  m_index_buffer_view_.BufferLocation = m_index_buffer_->GetGPUVirtualAddress();
  m_index_buffer_view_.Format = DXGI_FORMAT_R32_UINT;
  // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
  m_index_buffer_view_.SizeInBytes = index_buffer_size;

  return true;
}


void QuadRenderer::Release() {
  // Erst Resourcen dieser Klasse freigeben, dann die Resourcen der Basisklasse
  m_index_buffer_.Reset();
  TriangleRenderer::Release();
}
