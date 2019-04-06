#include "stdafx.h"
#include "DynTriangleRenderer.h"
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>


/*
	Resource Heaps are like descriptor heaps, but instead of storing descriptors, 
	they store the actual data for the resources. They are a chunk of allocated memory, 
	either on the GPU or on the CPU, depending on the type of heap. 

	Upload heaps	-> used to upload data to the GPU. The GPU has read access to this memory, while the CPU has write access
						vertex buffer is stored here and gets copied to the default heap
	Default heaps	-> Chunks of memory on the GPU. The CPU has no access to this memory, which makes accessing this memory in shaders very fast
						here are the resources stored that the shaders will use

	Input Layout (D3D12_INPUT_ELEMENT_DESC)	-> describes how the input assembler should read the vertices in the vertex buffer

*/

DynTriangleRenderer::DynTriangleRenderer(int vertexCount) {
  if (vertexCount < 3) {
    Log::Info("Less than 3 vertices submitted, using default value 3");
    vertexCount = 3;
  }
  m_vertex_count_ = vertexCount;
  m_triangle_count_ = vertexCount - 2;
}


bool DynTriangleRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height) {
  // Wird einmalig beim Start aufgerufen. Implementierung gem‰ﬂ Vorlesung

  // init viewport and scissorRect with default values
  m_viewport_ = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissor_rect_ = CD3DX12_RECT(0, 0, static_cast<float>(width), static_cast<float>(height));

  // create empty root signature
  if (!this->CreateRootSignature(device)) {
    Log::Error("Error create createroot signature");
    return false;
  }

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;

  if (!this->CompileShaders(vertexShader, "main", pixelShader, "main")) {
    Log::Error("Error compile shaders");
    return false;
  }

  // Define the vertex input layout.
  D3D12_INPUT_ELEMENT_DESC inputElementDescs [] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
  psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)}; // The format of your vertex structure
  psoDesc.pRootSignature = m_root_signature_.Get();
  // A root signature is basically a parameter list of data that the shader functions expect.
  // The shaders must be compatible with the Root Signature.
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
  // A D3D12_SHADER_BYTECODE structure that describes the vertex shader.
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
  // A D3D12_SHADER_BYTECODE structure that describes the pixel shader.
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  // Rasterizer state such as cull mode, wireframe/solid rendering, antialiasing, etc.
  // A D3D12_RASTERIZER_DESC structure.
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  // This describes the blend state that is used when the output merger writes a pixel fragment to the render target.
  psoDesc.DepthStencilState.DepthEnable = FALSE; // Used for depth/stencil testing
  psoDesc.DepthStencilState.StencilEnable = FALSE; // Used for depth/stencil testing
  psoDesc.SampleMask = UINT_MAX; // The sample mask for the blend state.
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // saying whether the Input Assembler should assemble the geometry as Points, Lines, Triangles, or Patches (used for tesselation). 
  // This is different from the adjacency and ordering that is set in the command list (triangle list, triangle strip, etc).
  psoDesc.NumRenderTargets = 1; // it is possible to write to more than one render target at a time.
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  // An array of DXGI_FORMAT-typed values for the render target formats.
  psoDesc.SampleDesc.Count = 1; // The number of multisamples per pixel.

  HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipeline_state_));
  if (FAILED(hr)) {
    Log::Error("Error create graphics pipeline state -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool DynTriangleRenderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList>& command_list,
                                         ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) {
  HRESULT hr;
  // Wird einmalig beim Start aufgerufen.
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


  struct coords {
    float x;
    float y;
  };

  std::vector<coords> pos;
  float x;
  float y;
  float r = 0.5f;
  // Calculate positions for all vertices
  for (int i = 0; i < m_vertex_count_; i++) {
    x = r * cos(2 * XM_PI * i / m_vertex_count_);
    y = r * sin(2 * XM_PI * i / m_vertex_count_);
    pos.insert(pos.begin(), {x, y});
  }

  std::vector<Vertex> triangleVertices;
  // Push vertices for all needed triangles
  for (int i = 1; i <= m_triangle_count_; i++) {
    triangleVertices.push_back({{pos.data()[0].x, pos.data()[0].y, 0.0f}, THM_GREEN}); // Root position
    triangleVertices.push_back({{pos.data()[i].x, pos.data()[i].y, 0.0f}, THM_GREEN}); // +1 position
    triangleVertices.push_back({{pos.data()[i + 1].x, pos.data()[i + 1].y, 0.0f}, THM_GREEN}); // +2 position
  }

  if (!this->CreateVertexBuffer(m_device, command_list, triangleVertices.data(),
                                sizeof(Vertex) * triangleVertices.size())) {
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


bool DynTriangleRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                                                ComPtr<ID3D12CommandAllocator>& command_allocator,
                                                CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                                                ComPtr<ID3D12Resource>& render_target, int frame_index) {
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
  command_list->DrawInstanced(m_triangle_count_ * 3, 1, 0, 0); // finally draw 3 vertices (draw the triangle)

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


bool DynTriangleRenderer::CreateVertexBuffer(ComPtr<ID3D12Device>& device,
                                             ComPtr<ID3D12GraphicsCommandList>& command_list, Vertex* vertex_list,
                                             int vertex_buffer_size) {
  // Sollte einmalig genutzt werden wenn Ressourcen geladen werden.

  // create default heap
  // default heap is memory on the GPU. Only the GPU has access to this memory
  // To get data into this heap, we will have to upload the data using
  // an upload heap
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size), // resource description for a buffer
    D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
    // from the upload heap to this heap
    nullptr,
    // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
    IID_PPV_ARGS(&m_vertex_buffer_));

  // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
  m_vertex_buffer_->SetName(L"Vertex Buffer Resource Heap");

  // create upload heap
  // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
  // We will upload the vertex buffer using this heap to the default heap
  ID3D12Resource* vertexBufferUploadHeap;
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size), // resource description for a buffer
    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
    nullptr,
    IID_PPV_ARGS(&vertexBufferUploadHeap));
  vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

  // store vertex buffer in upload heap
  D3D12_SUBRESOURCE_DATA vertexData = {};
  vertexData.pData = reinterpret_cast<BYTE*>(vertex_list); // pointer to our vertex array
  vertexData.RowPitch = vertex_buffer_size; // size of all our triangle vertex data
  vertexData.SlicePitch = vertex_buffer_size; // also the size of our triangle vertex data

  // we are now creating a command with the command list to copy the data from
  // the upload heap to the default heap
  UINT64 r = UpdateSubresources(command_list.Get(), m_vertex_buffer_.Get(), vertexBufferUploadHeap, 0, 0, 1, &vertexData);

  // transition the vertex buffer data from copy destination state to vertex buffer state
  command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer_.Get(),
                                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                                        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
  m_vertex_buffer_view_.BufferLocation = m_vertex_buffer_->GetGPUVirtualAddress();
  m_vertex_buffer_view_.StrideInBytes = sizeof(Vertex);
  m_vertex_buffer_view_.SizeInBytes = vertex_buffer_size;
  return true;
}


bool DynTriangleRenderer::CompileShaders(ComPtr<ID3DBlob>& vertex_shader, LPCSTR entry_point_vertex_shader,
                                         ComPtr<ID3D10Blob>& pixel_shader, LPCSTR entry_point_pixel_shader) {
  // Sollte einmalig genutzt werden bevor das PSO gesetzt wird.
#if defined(_DEBUG)
  // Enable better shader debugging with the graphics debugging tools.
  UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

  HRESULT hr = D3DCompileFromFile(L"vertex_shader.hlsl", nullptr, nullptr, entry_point_vertex_shader, "vs_5_0",
                                  compileFlags, 0, &vertex_shader, nullptr);
  if (FAILED(hr)) {
    Log::Error("Error decompile vertex shader -ERROR:" + std::to_string(hr));
    return false;
  }

  hr = D3DCompileFromFile(L"pixel_shader.hlsl", nullptr, nullptr, entry_point_pixel_shader, "ps_5_0", compileFlags, 0,
                          &pixel_shader, nullptr);
  if (FAILED(hr)) {
    Log::Error("Error decompile pixel shader -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool DynTriangleRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device) {
  // Sollte einmalig genutzt werden bevor das PSO gesetzt wird.
  // Create an empty root signature
  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  if (FAILED(hr)) {
    Log::Error("Error serialize root signature -ERROR:" + std::to_string(hr));
    return false;
  }
  hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                   IID_PPV_ARGS(&m_root_signature_));
  if (FAILED(hr)) {
    Log::Error("Error create root signature -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


void DynTriangleRenderer::Release() {
  // Erst Resourcen dieser Klasse freigeben, dann die Resourcen der Basisklasse
  m_root_signature_.Reset();
  m_pipeline_state_.Reset();
  m_vertex_buffer_.Reset();
  Renderer::Release();
}
