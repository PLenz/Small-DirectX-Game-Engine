#include "stdafx.h"
#include "ImageRenderer.h"
#include "Camera.h"
#include "ObjLoader.h"

bool ImageRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
	// Wird einmalig beim Start aufgerufen.

	// init viewport and scissorRect with default values
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
	m_scissorRect = CD3DX12_RECT(0, 0, static_cast<float>(width), static_cast<float>(height));

	// create empty root signature
	if(!this->CreateRootSignature(device)) {
		Log::Error("Error create root signature");
		return false;
	}

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	// compile the shaders. Second param for function declared in vertexShader.hlsl
	if(!this->CompileShaders(vertexShader, "rotate", pixelShader, "textured")) {
		Log::Error("Error compile shaders");
		return false;
	}

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC  inputElementDescs [] ={
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT , 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT , 0, 12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT , 0, 28,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
	};

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};							// a structure to define a pso

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };	// The format of your vertex structure
	psoDesc.pRootSignature = m_rootSignature.Get();								// A root signature is basically a parameter list of data that the shader functions expect. The shaders must be compatible with the Root Signature.
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());					// A D3D12_SHADER_BYTECODE structure that describes the vertex shader.
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());					// A D3D12_SHADER_BYTECODE structure that describes the pixel shader.
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);			// Rasterizer state such as cull mode, wireframe/solid rendering, antialiasing, etc. A D3D12_RASTERIZER_DESC structure.
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);						// This describes the blend state that is used when the output merger writes a pixel fragment to the render target.
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);		// Used for depth/stencil testing. Using default values of CD3DX12_DEPTH_STENCIL_DESC
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;									// A DXGI_FORMAT-typed value for the depth-stencil format.
	psoDesc.SampleMask = UINT_MAX;												// The sample mask for the blend state.
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// saying whether the Input Assembler should assemble the geometry as Points, Lines, Triangles, or Patches (used for tesselation). 
																				// This is different from the adjacency and ordering that is set in the command list (triangle list, triangle strip, etc).
	psoDesc.NumRenderTargets = 1;												// it is possible to write to more than one render target at a time.
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;							// An array of DXGI_FORMAT-typed values for the render target formats.
	psoDesc.SampleDesc.Count = 1;												// The number of multisamples per pixel.

	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr)) {
		Log::Error("Error create graphics pipeline state -ERROR:" + std::to_string(hr));
		return false;
	}
    return true;;
}

bool ImageRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
	HRESULT hr;
    // Wird einmalig beim Start aufgerufen.
    hr = commandAllocator->Reset();
	if (FAILED(hr))
	{
		Log::Error("Error commandAllocator->Reset() -ERROR:" + std::to_string(hr));
		return false;
	}

	// Reset the command list
	hr = commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());
	if (FAILED(hr))
	{
		Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
		return false;
	}

	Camera::SetAspectRatio(width, height);
		

	Vertex* triangleVertices;
	DWORD* iList;
	int vCount;
	BoundingVolume* boundingVolume;
	ObjLoader::Load("models/plane.obj", triangleVertices, vCount, iList, iListSize, boundingVolume);
	
	if (!this->CreateIndexBuffer(device, commandList, iList, sizeof(DWORD) * iListSize)) {
		Log::Error("Error create index buffer -ERROR:");
		return false;
	}	

	if (!this->CreateVertexBuffer(device, commandList, triangleVertices, sizeof(Vertex) * vCount)) {
		Log::Error("Error create vertex buffer -ERROR:");
		return false;
	}

	if (!this->CreateDepthStencilBuffer(device, commandList, width, height)) {
		Log::Error("Error create vdepth/stencil buffer -ERROR:");
		return false;
	}

	if (!this->CreateConstantBuffer(device)) {
		Log::Error("Error create constant buffer -ERROR:");
		return false;
	}

	if (!LoadTexture(device, commandList))
	{
		Log::Error("Error loading texture -ERROR:");
		return false;
	}

	hr = commandList->Close();
	if (FAILED(hr)) {
		Log::Error("Error close command list -ERROR:" + std::to_string(hr));
		return false;
	}
	return true;
}

bool ImageRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE & rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
{
	// Wird mit jedem Frame aufgerufen.
	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	HRESULT hr = commandAllocator->Reset();
	if (FAILED(hr))
	{
		Log::Error("Error commandAllocator->Reset() -ERROR:" + std::to_string(hr));
		return false;
	}

	// Reset the command list
	hr = commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());
	if (FAILED(hr))
	{
		Log::Error("Error commandList->Reset -ERROR:" + std::to_string(hr));
		return false;
	}

	commandList->SetGraphicsRootSignature(m_rootSignature.Get()); // set the root signature
	commandList->RSSetViewports(1, &m_viewport); // set the viewports
	commandList->RSSetScissorRects(1, &m_scissorRect); // set the scissor rects
	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

	// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// set the render target for the output merger stage (the output of the pipeline)
	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set the render target for the output merger stage (the output of the pipeline)
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear the render target by using the ClearRenderTargetView command
	const float clearColor[] = WHITE;
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	commandList->ClearDepthStencilView(m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// set constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap[frameIndex].Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
	commandList->SetGraphicsRootDescriptorTable(1, m_mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());

	// draw triangle
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferUploadHeap[frameIndex]->GetGPUVirtualAddress());
	commandList->DrawIndexedInstanced(iListSize, 1, 0, 0, 0); // finally draw 3 vertices (draw the first triangle)

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = commandList->Close();
	if (FAILED(hr))
	{
		Log::Error("Error close command list -ERROR:" + std::to_string(hr));
		return false;
	}
	return true;
}

void ImageRenderer::Update(int frameIndex, double delta)
{
	// update app logic, such as moving the camera or figuring out what objects are in view

	// update constant buffer
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat = XMLoadFloat4x4(&Camera::GetViewMatrix()); // load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&Camera::GetProjectionMatrix()); // load projection matrix
	XMMATRIX transposed = XMMatrixTranspose(viewMat * projMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&m_constantBuffer.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(m_constantBufferGPUAddress[frameIndex], &m_constantBuffer, sizeof(m_constantBuffer));
}

void ImageRenderer::Release()
{
	m_textureBuffer.Reset(); // the resource heap containing our texture
	m_textureBufferUploadHeap.Reset();
	delete m_logoData;
	DepthQuadRenderer::Release();
}

bool ImageRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
{
	HRESULT hr;
	// create a root descriptor, which explains where to find the data for this root parameter
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.RegisterSpace = 0;
    rootCBVDescriptor.ShaderRegister = 0;

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	descriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
	descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

    // create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

    // create a root parameter for the root descriptor and fill it out
	D3D12_ROOT_PARAMETER  rootParameters[2]; // two root parameters
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	// fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
    // buffer will be changed multiple times per frame, while our descriptor table will not be changed at all (in this tutorial)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[1].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter for now

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
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    ComPtr<ID3DBlob> errorBuff; // a buffer holding the error data if any
    ComPtr<ID3DBlob> signature;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
    if (FAILED(hr))
    {
        Log::Error("Error serialize root signature -ERROR:" + std::to_string(hr));
		return false;
    }

    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        Log::Error("Error create root signature -ERROR:" + std::to_string(hr));
		return false;
    }
	return true;
}

bool ImageRenderer::LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	HRESULT hr;
	// Load the image from file
	D3D12_RESOURCE_DESC textureDesc;
	int imageBytesPerRow;
	int imageSize = TextureLoader::LoadImageDataFromFile(&m_logoData, textureDesc, L"thm.png", imageBytesPerRow);
	// make sure we have data
	if (imageSize <= 0)
	{
		Log::Error("Image has no size -ERROR:");
		return false;
	}

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&textureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&m_textureBuffer));
	if (FAILED(hr))
	{
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
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&m_textureBufferUploadHeap));
	if (FAILED(hr))
	{
		Log::Error("Error create upload heap in LoadTexture -ERROR:" + std::to_string(hr));
		return false;
	}
	m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store texture buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &m_logoData[0]; // pointer to our image data
	textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
	textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(commandList.Get(), m_textureBuffer.Get(), m_textureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

	// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	for (int i = 0; i < DXGE_FRAME_COUNT; i++)
	{
		// create the descriptor heap that will store our srv
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap[i]));
		if (FAILED(hr))
		{
			Log::Error("Error create descriptor heap in LoadTexture -ERROR:" + std::to_string(hr));
			return false;
		}

		// now we create a shader resource view (descriptor that points to the texture and describes it)
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc, m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());

	}

	return true;
}
