#include "stdafx.h"
#include "Mesh.h"
#include "ObjLoader.h"
#include "TextureLoader.h"


bool Mesh::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                         ComPtr<ID3D12CommandAllocator>& commandAllocator) {
  if (!loaded) {
    HRESULT hr;
    // Wird einmalig beim Start aufgerufen.

    Vertex* triangleVertices;
    DWORD* iList;
    int vCount;
    ObjLoader::Load(std::string(m_objPath.begin(), m_objPath.end()), triangleVertices, vCount, iList, m_iListSize,
                    m_boundingVolume);

    if (!this->CreateIndexBuffer(device, commandList, iList, sizeof(DWORD) * m_iListSize)) {
      Log::Error("Error create index buffer -ERROR:");
      return false;
    }

    if (!this->CreateVertexBuffer(device, commandList, triangleVertices, sizeof(Vertex) * vCount)) {
      Log::Error("Error create vertex buffer -ERROR:");
      return false;
    }

    if (!LoadTexture(device, commandList)) {
      Log::Error("Error loading texture -ERROR:");
      return false;
    }
    loaded = true;
  }
  return true;
}


bool Mesh::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList,
                               D3D12_GPU_VIRTUAL_ADDRESS constantBufferUploadHeap) {
  // Wird mit jedem Frame aufgerufen.
  HRESULT hr;

  // set constant buffer descriptor heap
  ID3D12DescriptorHeap* descriptorHeaps[] = {m_shaderResourceViewDescriptorHeap.Get()};
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
  commandList->SetGraphicsRootDescriptorTable(
    1, m_shaderResourceViewDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
  commandList->IASetIndexBuffer(&m_indexBufferView);
  commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeap);
  commandList->DrawIndexedInstanced(m_iListSize, 1, 0, 0, 0); // finally draw 3 vertices (draw the first triangle)

  return true;
}


void Mesh::Update(int frameIndex, double delta) {}


void Mesh::Release() {
  m_vertexBuffer.Reset();
  m_indexBuffer.Reset();
  m_textureBuffer.Reset();
  m_textureBufferUploadHeap.Reset();
  m_shaderResourceViewDescriptorHeap.Reset();
  delete m_imageData;
}


int Mesh::getIListSize() {
  return m_iListSize;
}


BoundingVolume* Mesh::GetBoundingVolume() {
  return m_boundingVolume;
}


bool Mesh::CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, DWORD* iList,
                             int indexBufferSize) {
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
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                 m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  // create a index buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
  // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
  m_indexBufferView.SizeInBytes = indexBufferSize;

  return true;
}


bool Mesh::CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
                              Vertex* vList, int vertexBufferSize) {
  // Sollte einmalig genutzt werden wenn Ressourcen geladen werden. Implementierung gemäß Vorlesung.

  // create default heap
  // default heap is memory on the GPU. Only the GPU has access to this memory
  // To get data into this heap, we will have to upload the data using
  // an upload heap
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), // resource description for a buffer
    D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
    // from the upload heap to this heap
    nullptr,
    // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
    IID_PPV_ARGS(&m_vertexBuffer));

  // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
  m_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

  // create upload heap
  // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
  // We will upload the vertex buffer using this heap to the default heap
  ID3D12Resource* vertexBufferUploadHeap;
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), // resource description for a buffer
    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
    nullptr,
    IID_PPV_ARGS(&vertexBufferUploadHeap));
  vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

  // store vertex buffer in upload heap
  D3D12_SUBRESOURCE_DATA vertexData = {};
  vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
  vertexData.RowPitch = vertexBufferSize; // size of all our triangle vertex data
  vertexData.SlicePitch = vertexBufferSize; // also the size of our triangle vertex data

  // we are now creating a command with the command list to copy the data from
  // the upload heap to the default heap
  UINT64 r = UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), vertexBufferUploadHeap, 0, 0, 1,
                                &vertexData);

  // transition the vertex buffer data from copy destination state to vertex buffer state
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                 m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.StrideInBytes = sizeof(Vertex);
  m_vertexBufferView.SizeInBytes = vertexBufferSize;
  return true;
}


bool Mesh::LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList) {
  HRESULT hr;
  // Load the image from file
  D3D12_RESOURCE_DESC textureDesc;
  int imageBytesPerRow;

  int imageSize = TextureLoader::LoadImageDataFromFile(&m_imageData, textureDesc, m_texturePath.c_str(),
                                                       imageBytesPerRow);

  // make sure we have data
  if (imageSize <= 0) {
    Log::Error("Image has no size -ERROR:");
    return false;
  }

  // create a default heap where the upload heap will copy its contents into (contents being the texture)
  hr = device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &textureDesc, // the description of our texture
    D3D12_RESOURCE_STATE_COPY_DEST,
    // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
    nullptr, // used for render targets and depth/stencil buffers
    IID_PPV_ARGS(&m_textureBuffer));
  if (FAILED(hr)) {
    Log::Error("Error create default heap in LoadTexture -ERROR:" + std::to_string(hr));
    return false;
  }
  m_textureBuffer->SetName(L"Texture Buffer Resource Heap");

  UINT64 textureUploadBufferSize;
  // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
  // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
  // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
  //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
  device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

  // now we create an upload heap to upload our texture to the GPU
  hr = device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
    D3D12_HEAP_FLAG_NONE, // no flags
    &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
    // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
    D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
    nullptr,
    IID_PPV_ARGS(&m_textureBufferUploadHeap));
  if (FAILED(hr)) {
    Log::Error("Error create upload heap in LoadTexture -ERROR:" + std::to_string(hr));
    return false;
  }
  m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

  // store texture buffer in upload heap
  D3D12_SUBRESOURCE_DATA textureData = {};
  textureData.pData = &m_imageData[0]; // pointer to our image data
  textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
  textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

  // Now we copy the upload buffer contents to the default heap
  UpdateSubresources(commandList.Get(), m_textureBuffer.Get(), m_textureBufferUploadHeap.Get(), 0, 0, 1,
                     &textureData);

  // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                 m_textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

  // create the descriptor heap that will store our srv
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = 1;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_shaderResourceViewDescriptorHeap));
  if (FAILED(hr)) {
    Log::Error("Error create descriptor heap in LoadTexture -ERROR:" + std::to_string(hr));
    return false;
  }

  // now we create a shader resource view (descriptor that points to the texture and describes it)
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format = textureDesc.Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc,
                                   m_shaderResourceViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  return true;
}


bool Mesh::Compare(const Mesh& r) {
  if (m_objPath.compare(r.m_objPath) != 0) {
    return false;
  }

  if (m_texturePath.compare(r.m_texturePath) != 0) {
    return false;
  }

  return true;
}
