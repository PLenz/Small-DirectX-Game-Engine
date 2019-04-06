#include "stdafx.h"
#include "TriangleRenderer.h"

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

bool TriangleRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
    // Wird einmalig beim Start aufgerufen. Implementierung gem‰ﬂ Vorlesung

	// init viewport and scissorRect with default values
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
	m_scissorRect = CD3DX12_RECT(0, 0, static_cast<float>(width), static_cast<float>(height));

	// create empty root signature
	if(!this->CreateRootSignature(device)) {
		Log::Error("Error create createroot signature");
		return false;
	}

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	if(!this->CompileShaders(vertexShader, "main", pixelShader, "main")) {
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
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) }; // The format of your vertex structure
	psoDesc.pRootSignature = m_rootSignature.Get(); // A root signature is basically a parameter list of data that the shader functions expect. 
													// The shaders must be compatible with the Root Signature.
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());	// A D3D12_SHADER_BYTECODE structure that describes the vertex shader.	
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());	// A D3D12_SHADER_BYTECODE structure that describes the pixel shader.
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);	// Rasterizer state such as cull mode, wireframe/solid rendering, antialiasing, etc.
																		// A D3D12_RASTERIZER_DESC structure.
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);	// This describes the blend state that is used when the output merger writes a pixel fragment to the render target.
	psoDesc.DepthStencilState.DepthEnable = FALSE;		// Used for depth/stencil testing
	psoDesc.DepthStencilState.StencilEnable = FALSE;	// Used for depth/stencil testing
	psoDesc.SampleMask = UINT_MAX;	// The sample mask for the blend state.
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// saying whether the Input Assembler should assemble the geometry as
																			// Points, Lines, Triangles, or Patches (used for tesselation). 
																			// This is different from the adjacency and ordering that is set 
																			// in the command list (triangle list, triangle strip, etc).
	psoDesc.NumRenderTargets = 1;		// it is possible to write to more than one render target at a time.
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// An array of DXGI_FORMAT-typed values for the render target formats.
	psoDesc.SampleDesc.Count = 1;	// The number of multisamples per pixel.

	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr)) {
		Log::Error("Error create graphics pipeline state -ERROR:" + std::to_string(hr));
		return false;
	}
    return true;
}

bool TriangleRenderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, int width, int height)
{
	HRESULT hr;
    // Reset the command allocator
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
        { {	-0.25f,	0.25f,	0.0f },	THM_GREEN },
		{ {	0.25f,	-0.25f, 0.0f },	THM_GREEN },
		{ {	-0.25f,	-0.25f, 0.0f },	THM_GREEN },

		{ { 0.25f,	-0.25f,	0.0f }, THM_RED },
		{ { -0.25f,	0.25f,	0.0f },	THM_RED },
		{ { 0.25f,	0.25f,	0.0f },	THM_RED },
	};

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

bool TriangleRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12CommandAllocator> &commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE &rtvHandle, ComPtr<ID3D12Resource> &renderTarget, int frameIndex)
{
    // Wird mit jedem Frame aufgerufen
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
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Clear the render target by using the ClearRenderTargetView command
    const float clearColor[] = THM_GREY;
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // draw triangle
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
    commandList->DrawInstanced(6, 1, 0, 0); // finally draw 3 vertices (draw the triangle)

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

bool TriangleRenderer::CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList> &commandList, Vertex* vList, int vertexBufferSize)
{
    // Sollte einmalig genutzt werden wenn Ressourcen geladen werden. Implementierung gem‰ﬂ Vorlesung.

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
		nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
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
    UINT64 r = UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), vertexBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
    return true;
}

bool TriangleRenderer::CompileShaders(ComPtr<ID3DBlob>& vertexShader, LPCSTR entryPointVertexShader, ComPtr<ID3D10Blob>& pixelShader, LPCSTR entryPointPixelShader)
{
    // Sollte einmalig genutzt werden bevor das PSO gesetzt wird.
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

	HRESULT hr = D3DCompileFromFile(L"vertexShader.hlsl", nullptr, nullptr, entryPointVertexShader, "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
	if (FAILED(hr)) {
		Log::Error("Error decompile vertex shader -ERROR:" + std::to_string(hr));
		return false;
	}

    hr = D3DCompileFromFile(L"pixelShader.hlsl", nullptr, nullptr, entryPointPixelShader, "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
	if (FAILED(hr)) {
		Log::Error("Error decompile pixel shader -ERROR:" + std::to_string(hr));
		return false;
	}
    return true;
}

bool TriangleRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
{
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
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hr)) {
		Log::Error("Error create root signature -ERROR:" + std::to_string(hr));
		return false;
	}
    return true;
}

void TriangleRenderer::Release()
{
   // Erst Resourcen dieser Klasse freigeben, dann die Resourcen der Basisklasse
   m_rootSignature.Reset();
   m_pipelineState.Reset();
   m_vertexBuffer.Reset();
   Renderer::Release();
}
