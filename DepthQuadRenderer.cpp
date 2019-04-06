#include "stdafx.h"
#include "DepthQuadRenderer.h"
#include "Camera.h"

bool DepthQuadRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
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
	if(!this->CompileShaders(vertexShader, "rotate", pixelShader, "main")) {
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
    return true;
}

bool DepthQuadRenderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, int width, int height)
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

	// create triangle vertices
	Vertex triangleVertices[] =	
	{
		// first quad (in front)
		{ {-0.25f,  0.25f,  0.2f},THM_GREEN }, // LO 0
		{ {0.25f,  -0.25f, 0.2f},THM_GREEN }, // RU 1
		{ {-0.25f,  -0.25f, 0.2f},THM_GREEN }, // LU 2
		{ { 0.25f,  0.25f,  0.2f},THM_GREEN }, // RO 3

		// second quad (back)
		{ {0.0f,  0.0f,  0.5f},		THM_RED }, // LO 4
		{ {0.5f,  -0.5f, 0.5f},		THM_RED }, // RU 5
		{ {0.0f,  -0.5f, 0.5f},		THM_RED }, // LU 6
		{ { 0.5f,  0.0f, 0.5f},		THM_RED }  // RO 7
	};

	// a quad (2 triangles)
    DWORD iList[] = {
        0, 1, 2, // first triangle
		2, 1, 0, // first triangle back
        0, 3, 1, // second triangle
		1, 3, 0, // second triangle back
    };

	iListSize = _countof(iList);

	Camera::SetAspectRatio(width, height);

	if (!this->CreateVertexBuffer(m_device, commandList, triangleVertices, sizeof(triangleVertices))) {
		Log::Error("Error create vertex buffer -ERROR:");
		return false;
	}

	if (!this->CreateIndexBuffer(m_device, commandList, iList, sizeof(iList))) {
		Log::Error("Error create index buffer -ERROR:");
		return false;
	}	

	if (!this->CreateDepthStencilBuffer(m_device, commandList, width, height)) {
		Log::Error("Error create vdepth/stencil buffer -ERROR:");
		return false;
	}

	if (!this->CreateConstantBuffer(m_device)) {
		Log::Error("Error create constant buffer -ERROR:");
		return false;
	}

	hr = commandList->Close();
	if (FAILED(hr)) {
		Log::Error("Error close command list -ERROR:" + std::to_string(hr));
		return false;
	}
	return true;
}

bool DepthQuadRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
{
	HRESULT hr;
	// create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
    descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
    descriptorTableRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
    descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
    descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables
    
    // create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
    descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
    descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

    // create a root parameter and fill it out
    D3D12_ROOT_PARAMETER  rootParameters[1]; // only one parameter right now
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[0].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
        rootParameters, // a pointer to the beginning of our root parameters array
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

    ID3DBlob* signature;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
    if (FAILED(hr))
    {
        return false;
    }

    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        return false;
    }
	return true;
}

bool DepthQuadRenderer::CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, int width, int height)
{
	HRESULT hr;
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthStencilDescriptorHeap));
	if (FAILED(hr))
	{
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
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&m_depthStencilBuffer)
        );
    m_depthStencilDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool DepthQuadRenderer::CreateConstantBuffer(ComPtr<ID3D12Device>& device)
{
	HRESULT hr;
	// Create a constant buffer descriptor heap for each frame
	// this is the descriptor heap that will store our constant buffer descriptor
	for (int i = 0; i < DXGE_FRAME_COUNT; ++i)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap[i]));
		if (FAILED(hr))
		{
			Log::Error("Error create constant buffer heap -ERROR:" + std::to_string(hr));
			return false;
		}
	}

	// create the constant buffer resource heap
	// We will update the constant buffer one or more times per frame, so we will use only an upload heap
	// unlike previously we used an upload heap to upload the vertex and index data, and then copied over
	// to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
	// efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
	// will be modified and uploaded at least once per frame, so we only use an upload heap

	// create a resource heap, descriptor heap, and pointer to cbv for each frame
	for (int i = 0; i < DXGE_FRAME_COUNT; ++i)
	{
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&m_constantBufferUploadHeap[i]));
		m_constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBufferUploadHeap[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;    // CB size is required to be 256-byte aligned.
		device->CreateConstantBufferView(&cbvDesc, m_mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());

		ZeroMemory(&m_constantBuffer, sizeof(m_constantBuffer));

		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
		hr = m_constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferGPUAddress[i]));
		memcpy(m_constantBufferGPUAddress[i], &m_constantBufferGPUAddress, sizeof(m_constantBufferGPUAddress));
	}
	return true;
}

bool DepthQuadRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE &rtvHandle, ComPtr<ID3D12Resource> &renderTarget, int frameIndex)
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
	const float clearColor[] = THM_GREY;
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	commandList->ClearDepthStencilView(m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// set constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap[frameIndex].Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// set the root descriptor table 0 to the constant buffer descriptor heap
	commandList->SetGraphicsRootDescriptorTable(0, m_mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());

	// draw triangle
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
	commandList->IASetIndexBuffer(&m_indexBufferView);
	commandList->DrawIndexedInstanced(iListSize, 1, 0, 0, 0); // finally draw 3 vertices (draw the first triangle)
	commandList->DrawIndexedInstanced(iListSize, 1, 0, 4, 0); // finally draw 3 vertices (draw the first triangle)

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

void DepthQuadRenderer::Update(int frameIndex, double delta)
{
	// update app logic, such as moving the camera or figuring out what objects are in view
	static float rIncrement = 0.0001f;
	static float gIncrement = 0.0001f;
	static float bIncrement = 0.0001f;
	static float angleIncrement = 0.1f;

	m_constantBuffer.factor.x += rIncrement * delta;
	m_constantBuffer.factor.y += gIncrement * delta;
	m_constantBuffer.factor.z += bIncrement * delta;

	if (m_constantBuffer.factor.x >= 1.0 || m_constantBuffer.factor.x <= 0.0)
	{
		m_constantBuffer.factor.x = m_constantBuffer.factor.x >= 1.0 ? 1.0 : 0.0;
		rIncrement = -rIncrement;
	}
	if (m_constantBuffer.factor.y >= 1.0 || m_constantBuffer.factor.y <= 0.0)
	{
		m_constantBuffer.factor.y = m_constantBuffer.factor.y >= 1.0 ? 1.0 : 0.0;
		gIncrement = -gIncrement;
	}
	if (m_constantBuffer.factor.z >= 1.0 || m_constantBuffer.factor.z <= 0.0)
	{
		m_constantBuffer.factor.z = m_constantBuffer.factor.z >= 1.0 ? 1.0 : 0.0;
		bIncrement = -bIncrement;
	}

	angle += (angleIncrement * delta);
	if (angle >= 360.0) {
		angle = 0.0;
	}

	Camera::Update(delta);

	// update constant buffer
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat = XMLoadFloat4x4(&Camera::GetViewMatrix()); // load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&Camera::GetProjectionMatrix()); // load projection matrix
	XMMATRIX transposed = XMMatrixTranspose(viewMat * projMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&m_constantBuffer.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(m_constantBufferGPUAddress[frameIndex], &m_constantBuffer, sizeof(m_constantBuffer));
}

void DepthQuadRenderer::Release()
{
	m_depthStencilBuffer.Reset();
	m_depthStencilDescriptorHeap.Reset();
	for (int i = 0; i < DXGE_FRAME_COUNT; i++) {
		m_mainDescriptorHeap[i].Reset();
		m_constantBufferUploadHeap[i].Reset();
	}
	QuadRenderer::Release();
}

