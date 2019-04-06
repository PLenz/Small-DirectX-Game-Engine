#include "stdafx.h"
#include "WorldRenderer.h"
#include "ImageRenderer.h"
#include "Camera.h"
#include "ObjLoader.h"
#include "LevelLoader.h"
#include "Mesh.h"

const auto offset = ((sizeof(ConstantBuffer) + 255) & ~255);


bool WorldRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height) {
  // Wird einmalig beim Start aufgerufen.

  // init viewport and scissorRect with default values
  m_viewport_ = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissor_rect_ = CD3DX12_RECT(0, 0, static_cast<float>(width), static_cast<float>(height));

  // create empty root signature
  if (!this->CreateRootSignature(device)) {
    Log::Error("Error create root signature");
    return false;
  }

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;

  // compile the shaders. Second param for function declared in vertexShader.hlsl
  if (!this->CompileShaders(vertexShader, "rotate", pixelShader, "textured")) {
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
  // A root signature is basically a parameter list of data that the shader functions expect. The shaders must be compatible with the Root Signature.
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
  // A D3D12_SHADER_BYTECODE structure that describes the vertex shader.
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
  // A D3D12_SHADER_BYTECODE structure that describes the pixel shader.
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  // Rasterizer state such as cull mode, wireframe/solid rendering, antialiasing, etc. A D3D12_RASTERIZER_DESC structure.
  psoDesc.RasterizerState.FrontCounterClockwise = true;
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  // This describes the blend state that is used when the output merger writes a pixel fragment to the render target.
  psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  // Used for depth/stencil testing. Using default values of CD3DX12_DEPTH_STENCIL_DESC
  psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // A DXGI_FORMAT-typed value for the depth-stencil format.
  psoDesc.SampleMask = UINT_MAX; // The sample mask for the blend state.
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // saying whether the Input Assembler should assemble the geometry as Points, Lines, Triangles, or Patches (used for tesselation). 
  // This is different from the adjacency and ordering that is set in the command list (triangle list, triangle strip, etc).
  psoDesc.NumRenderTargets = 1; // it is possible to write to more than one render target at a time.
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  // An array of DXGI_FORMAT-typed values for the render target formats.
  psoDesc.SampleDesc.Count = 1; // The number of multisamples per pixel.

  HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
  if (FAILED(hr)) {
    Log::Error("Error create graphics pipeline state -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool WorldRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                                   ComPtr<ID3D12CommandAllocator>& command_allocator, int width, int height) {

  vector<vector<int>> levelLayout = LevelLoader::Load("models/Level2.txt");
  for (float line = 0; line < levelLayout.size(); ++line) {
    for (float col = 0; col < levelLayout[line].size(); ++col) {

      XMFLOAT3 position = {line * 2.0f, 0, col * 2.0f};
      XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};

      switch (levelLayout[col][line]) {
        case 0:
          m_models_.push_back(Model(L"models/Wall.obj", L"models/Bake_Wall.png", position, rotation, true));
          break;
        case 1:
          m_models_.push_back(Model(L"models/Floor.obj", L"models/Bake_Floor.png", position, rotation, false));
          break;
        case 2:
          XMFLOAT4 rotat;
          XMStoreFloat4(&rotat, XMQuaternionRotationAxis(XMLoadFloat3(new XMFLOAT3(0.0f, 1.0f, 0.0f)), 0.25f * XM_PI));
          m_models_.push_back(Model(L"models/Floor.obj", L"models/Bake_Floor.png", position, rotation, false));
          m_models_.push_back(Model(L"models/Barrier.obj", L"models/Bake_Barrier.png", position, rotat, true));
          break;
        default:
          Log::Error("No object for level found! Object number" + levelLayout[col][line]);
          break;
      }
    }
  }

  HRESULT hr;
  // Wird einmalig beim Start aufgerufen.
  hr = command_allocator->Reset();
  if (FAILED(hr)) {
    Log::Error("Error command_allocator->Reset() -ERROR:" + std::to_string(hr));
    return false;
  }

  // Reset the command list
  hr = commandList->Reset(command_allocator.Get(), m_pipeline_state_.Get());
  if (FAILED(hr)) {
    Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
    return false;
  }

  Camera::SetAspectRatio(width, height);

  // CONSTANT BUFFER MUST CREATED BEFORE MESH LOAD RESOURCES
  if (!this->CreateConstantBuffer(device)) {
    Log::Error("Error create constant buffer -ERROR:");
    return false;
  }
  if (!this->CreateDepthStencilBuffer(device, commandList, width, height)) {
    Log::Error("Error create vdepth/stencil buffer -ERROR:");
    return false;
  }

  for (size_t modelIndex = 0; modelIndex < m_models_.size(); ++modelIndex) {
    m_models_[modelIndex].LoadResources(
      device,
      commandList,
      command_allocator
    );
  }

  hr = commandList->Close();
  if (FAILED(hr)) {
    Log::Error("Error close command list -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool WorldRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list,
                                          ComPtr<ID3D12CommandAllocator>& command_allocator,
                                          CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv_handle,
                                          ComPtr<ID3D12Resource>& render_target,
                                          int frameIndex) {
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

  // cullABB & reload released Meshes after reset.
  std::vector<Model*> culledModels;
  FrustumCulling(culledModels, command_list, command_allocator);

  command_list->SetGraphicsRootSignature(m_root_signature_.Get()); // set the root signature
  command_list->RSSetViewports(1, &m_viewport_); // set the viewports
  command_list->RSSetScissorRects(1, &m_scissor_rect_); // set the scissor rects
  // here we start recording commands into the command_list (which all the commands will be stored in the command_allocator)

  // transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
  command_list->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(render_target.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET));

  // set the render target for the output merger stage (the output of the pipeline)
  // get a handle to the depth/stencil buffer
  CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_depth_stencil_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

  // set the render target for the output merger stage (the output of the pipeline)
  command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsvHandle);

  // Clear the render target by using the ClearRenderTargetView command
  const float clearColor[] = THM_GREY;
  command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

  // clear the depth/stencil buffer
  command_list->ClearDepthStencilView(m_depth_stencil_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
                                     D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology

  for (size_t modelIndex = 0; modelIndex < culledModels.size(); ++modelIndex) {
    if (culledModels[modelIndex]->m_reload_ == true) {
      culledModels[modelIndex]->m_reload_ = false;
    } else {
      culledModels[modelIndex]->PopulateCommandList(
        command_list,
        p_constant_buffer_gpu_address_[frameIndex] + offset * modelIndex,
        m_constant_buffer_upload_heap_[frameIndex]->GetGPUVirtualAddress() + offset * modelIndex
      );
    }
  }

  // transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
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


void WorldRenderer::Update(int frame_index, double delta) {
  for (size_t modelIndex = 0; modelIndex < m_models_.size(); ++modelIndex) {
    m_models_[modelIndex].Update(delta);
  }
  Camera::Update(delta);
}


bool WorldRenderer::Intersects(BoundingVolume& camera, XMFLOAT3& resolution) {
  for (int i = 0; i < m_models_.size(); i++) {
    if (m_models_[i].Intersects(camera, resolution)) {
      return true;
    }
  }
  return false;
}


bool WorldRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device) {
  HRESULT hr;
  // create a root descriptor, which explains where to find the data for this root parameter
  D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
  rootCBVDescriptor.RegisterSpace = 0;
  rootCBVDescriptor.ShaderRegister = 0;

  // create a descriptor range (descriptor table) and fill it out
  // this is a range of descriptors inside a descriptor heap
  D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1]; // only one range right now
  descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  // this is a range of shader resource views (descriptors)
  descriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
  descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
  descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
  descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  // this appends the range to the end of the root signature descriptor tables

  // create a descriptor table
  D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
  descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
  descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

  // create a root parameter for the root descriptor and fill it out
  D3D12_ROOT_PARAMETER rootParameters[2]; // two root parameters
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
  rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  // our pixel shader will be the only shader accessing this parameter for now

  // fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
  // buffer will be changed multiple times per frame, while our descriptor table will not be changed at all (in this tutorial)
  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
  rootParameters[1].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
  rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  // our pixel shader will be the only shader accessing this parameter for now

  // create a static sampler
  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.MipLODBias = 0;
  sampler.MaxAnisotropy = 0;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister = 0;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init(_countof(rootParameters), // we have 2 root parameters
                         rootParameters, // a pointer to the beginning of our root parameters array
                         1, // we have one static sampler
                         &sampler, // a pointer to our static sampler (array)
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                         // we can deny shader stages here for better performance
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

  ComPtr<ID3DBlob> errorBuff; // a buffer holding the error data if any
  ComPtr<ID3DBlob> signature;
  hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
  if (FAILED(hr)) {
    Log::Error("Error serialize root signature -ERROR:" + std::to_string(hr));
    return false;
  }

  hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
  if (FAILED(hr)) {
    Log::Error("Error create root signature -ERROR:" + std::to_string(hr));
    return false;
  }
  return true;
}


bool WorldRenderer::CompileShaders(ComPtr<ID3DBlob>& vertex_shader, LPCSTR entry_point_vertex_shader,
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


bool WorldRenderer::CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device,
                                             ComPtr<ID3D12GraphicsCommandList>& command_list, int width, int height) {
  HRESULT hr;
  // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthStencilDescriptorHeap));
  if (FAILED(hr)) {
    Log::Error("Error creating depth/stencil descriptor heap -ERROR:" + std::to_string(hr));
    return false;
  }

  D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
  depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
  depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

  D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
  depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
  depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
  depthOptimizedClearValue.DepthStencil.Stencil = 0;

  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 1, 1, 0,
                                  D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &depthOptimizedClearValue,
    IID_PPV_ARGS(&m_depthStencilBuffer)
  );
  m_depth_stencil_descriptor_heap_->SetName(L"Depth/Stencil Resource Heap");

  device->CreateDepthStencilView(m_depth_stencil_buffer_.Get(), &depthStencilDesc,
                                 m_depth_stencil_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

  return true;
}


bool WorldRenderer::CreateConstantBuffer(ComPtr<ID3D12Device>& device) {
  HRESULT hr;
  // Create a constant buffer descriptor heap for each frame
  // this is the descriptor heap that will store our constant buffer descriptor
  for (int i = 0; i < DXGE_FRAME_COUNT; ++i) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_constantBufferDescriptorHeap[i]));
    if (FAILED(hr)) {
      Log::Error("Error create constant buffer heap -ERROR:" + std::to_string(hr));
      return false;
    }
  }

  // create the constant buffer resource heap
  // We will Update the constant buffer one or more times per frame, so we will use only an upload heap
  // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
  // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
  // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
  // will be modified and uploaded at least once per frame, so we only use an upload heap

  // create a resource heap, descriptor heap, and pointer to cbv for each frame
  for (int i = 0; i < DXGE_FRAME_COUNT; ++i) {
    hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
      D3D12_HEAP_FLAG_NONE, // no flags
      &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
      // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
      D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
      nullptr, // we do not have use an optimized clear value for constant buffers
      IID_PPV_ARGS(&m_constantBufferUploadHeap[i]));
    m_constant_buffer_upload_heap_[i]->SetName(L"Constant Buffer Upload Resource Heap");

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constant_buffer_upload_heap_[i]->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = offset; // CB size is required to be 256-byte aligned.
    device->CreateConstantBufferView(&cbvDesc, m_constant_buffer_descriptor_heap_[i]->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE readRange(0, 0);
    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
    hr = m_constant_buffer_upload_heap_[i]->Map(0, &readRange, reinterpret_cast<void**>(&p_constant_buffer_gpu_address_[i]));
    memcpy(p_constant_buffer_gpu_address_[i], &p_constant_buffer_gpu_address_, sizeof(p_constant_buffer_gpu_address_));
  }
  return true;
}


bool WorldRenderer::FrustumCulling(std::vector<Model*>& culled_models, ComPtr<ID3D12GraphicsCommandList>& command_list,
                             ComPtr<ID3D12CommandAllocator>& command_allocator) {
  BoundingVolume* frustrum;
  Camera::GetFrustum(frustrum);
  XMFLOAT3 pos = Camera::GetCameraPosition();
  float pitch = Camera::GetPitch();
  float yaw = Camera::GetYaw();

  XMVECTOR quaternionRotation = XMQuaternionMultiply(XMQuaternionRotationRollPitchYaw(pitch, yaw, 0),
                                                     XMQuaternionIdentity());
  XMFLOAT4 rot;
  XMStoreFloat4(&rot, quaternionRotation);

  frustrum->Update(&pos, &rot);

  int oldElementsToDraw = m_num_elements_to_draw_;
  m_num_elements_to_draw_ = 0;
  m_num_meshes_to_draw_ = 0;
  std::map<Mesh*, bool> meshAmountTable;

  for (int i = 0; i < m_models_.size(); ++i) {
    bool cull = m_models_[i].CheckCull(frustrum);
    m_models_[i].SetCull(cull, command_list, command_allocator);
    if (cull) {
      culled_models.push_back(&m_models_[i]);
      m_num_elements_to_draw_++;
      Mesh* mesh = m_models_[i].GetMesh();
      if (meshAmountTable.find(mesh) == meshAmountTable.end()) {
        meshAmountTable[mesh] = true;
        m_num_meshes_to_draw_++;
      }
    }
  }
  if (oldElementsToDraw != m_num_elements_to_draw_) {
    Log::Info("Total amount of Models: " + std::to_string(m_models_.size()));
    Log::Info("Currently rendered Models: " + std::to_string(m_num_elements_to_draw_));
    Log::Info("Total amount of Meshes: " + std::to_string(MeshCache::GetMeshTableSize()));
    Log::Info("Currently rendered Meshes: " + std::to_string(m_num_meshes_to_draw_));
  }

  return true;
}


void WorldRenderer::Release() {

  m_root_signature_.Reset();
  m_pipeline_state_.Reset();
  m_depth_stencil_buffer_.Reset();
  m_depth_stencil_descriptor_heap_.Reset();

  for (size_t i = 0; i < DXGE_FRAME_COUNT; i++) {
    m_constant_buffer_upload_heap_[i].Reset();
  }

  for (size_t modelIndex = 0; modelIndex < m_models_.size(); ++modelIndex) {
    m_models_[modelIndex].Unload();
    if (modelIndex == m_models_.size() - 1) {
      m_models_[modelIndex].Release();
    }
  }

  Renderer::Release();
}
