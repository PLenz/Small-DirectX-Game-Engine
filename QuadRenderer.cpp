#include "stdafx.h"
#include "QuadRenderer.h"


bool QuadRenderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                                 ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height) {
  HRESULT hr;
  // Wird einmalig beim Start aufgerufen. Implementierung gemäß Vorlesung
  hr = commandAllocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error commandAllocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());
  if (FAILED(hr)) {
    Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
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

  iListSize = _countof(iList);

  if (!this->CreateIndexBuffer(m_device, commandList, iList, sizeof(iList))) {
    Log::Error("Error create index buffer -ERROR:");
    return false;
  }

  if (!this->CreateVertexBuffer(m_device, commandList, triangleVertices, sizeof(triangleVertices))) {
    Log::Error("Error create vertex buffer -ERROR:");
    return false;
  }

  hr = commandList->Close();
  if (FAILED(hr)) {
    Log::Error("Error close command list -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool QuadRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                                       ComPtr<ID3D12CommandAllocator>& commandAllocator,
                                       CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget,
                                       int frameIndex) {
  // Wird mit jedem Frame aufgerufen.
  // we can only reset an allocator once the gpu is done with it
  // resetting an allocator frees the memory that the command list was stored in
  HRESULT hr = commandAllocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error commandAllocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());
  if (FAILED(hr)) {
    Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  commandList->SetGraphicsRootSignature(m_rootSignature.Get()); // set the root signature
  commandList->RSSetViewports(1, &m_viewport); // set the viewports
  commandList->RSSetScissorRects(1, &m_scissorRect); // set the scissor rects
  // here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

  // transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
  commandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET));

  // set the render target for the output merger stage (the output of the pipeline)
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  // Clear the render target by using the ClearRenderTargetView command
  const float clearColor[] = THM_GREY;
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  // draw triangle
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->DrawIndexedInstanced(iListSize, 1, 0, 0, 0); // finally draw 3 vertices (draw the triangle)

  // transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
  // warning if present is called on the render target when it's not in the present state
  commandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT));

  hr = commandList->Close();
  if (FAILED(hr)) {
    Log::Error("Error close command list -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool QuadRenderer::CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                                     DWORD* iList, int indexBufferSize) {
  // create default heap to hold index buffer
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize), // resource description for a buffer
    D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
    nullptr, // optimized clear value must be null for this type of resource
    IID_PPV_ARGS(&m_indexBuffer));

  // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
  m_indexBuffer->SetName(L"Index Buffer Resource Heap");

  // create upload heap to upload index buffer
  ID3D12Resource* iBufferUploadHeap;
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize), // resource description for a buffer
    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
    nullptr,
    IID_PPV_ARGS(&iBufferUploadHeap));
  iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

  // store index buffer in upload heap
  D3D12_SUBRESOURCE_DATA indexData = {};
  indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
  indexData.RowPitch = indexBufferSize; // size of all our index buffer
  indexData.SlicePitch = indexBufferSize; // also the size of our index buffer

  // we are now creating a command with the command list to copy the data from
  // the upload heap to the default heap
  UpdateSubresources(commandList.Get(), m_indexBuffer.Get(), iBufferUploadHeap, 0, 0, 1, &indexData);

  // transition the vertex buffer data from copy destination state to vertex buffer state
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(),
                                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  // create a index buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
  // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
  m_indexBufferView.SizeInBytes = indexBufferSize;

  return true;
}


void QuadRenderer::Release() {
  // Erst Resourcen dieser Klasse freigeben, dann die Resourcen der Basisklasse
  m_indexBuffer.Reset();
  TriangleRenderer::Release();
}
