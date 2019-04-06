#include "stdafx.h"
#include "DeviceManager.h"

/*
	A device is a virtual adapter used for creating
	 - Command Lists          -> used to allocate commands we want to execute on the GPU
	 - Command Allocators     -> Used to store the commands on the GPU
	 - Pipeline State Objects -> Used for setting pipeline state that determine how input data is interpreted and rendered
	 - Root Signatures		  -> Defines the data (resources) that shaders access
	 - Command Queues         -> Used for submitting Command Lists to be executed by the CPU or update resource tile mappings
	 - Fences                 -> Used to synchronize CPU and GPU
	 - Resources			  -> contain the data used to build your scene. They are chunks of memory that store geometry, textures, and shader data, where the graphics pipeline can access them
	 - Descriptors			  -> a structure which tells shaders where to find the resource, and how to interpret the data in the resource
	 - Descriptor Heaps		  -> a list of descriptors. They are a chunk of memory where the descriptors are stored.

	Resource Barriers are used to change the state or usage of a resource or subresources and to help synchronize the use of resources between multiple threads

	A factory is used to enumerate the devices

	Fences are used to synchronize CPU and GPU. The Command List associated with the Command Allocator MUST be finished executing on the CPU before
	we call CommandAllocator.reset(). Fences can help with that one.

	Fence Value is a value we told the Command List to set it to. Set in Populate command list

	Fence Event will be triggered once the Fence Value has been incremented by the last command of the command list. The last command will be injected by Signal()
	which will increment the Fence Value. That makes it visible that all previous commands have been processed

	CommandQueue.Execute() -> CommandList can be resetted directly after
*/


HRESULT hr;

bool DeviceManager::CreateDevice(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory)
{

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
	// Enable additional debug layers.
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // (_DEBUG) {}

	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		Log::Error("Error occured on creating factory -ERROR:" + std::to_string(hr));
	}

	// Get the hardware Adapter
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	IDXGIFactory2* pFactory = factory.Get();
	ComPtr<IDXGIAdapter1> adapter;
	bool adapterFound = false;
	std::wstring usedCard;
	std::wstring possibleCard;

	Log::Info("Grafikkarten:");
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// DeviceId 1046 is hardcoded for IntelHD graphics card
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE || desc.DeviceId == 1046)
		{
			possibleCard = desc.Description;
			Log::Info(L"Possible graphic card: " + possibleCard);
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			adapter.Reset();
			continue;
		}
		possibleCard = desc.Description;
		Log::Info(L"Possible graphic card: " + possibleCard);

		if (!adapterFound) {
			// Try to create the found device. If an error occures, use the warp adapter
			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
			if (SUCCEEDED(hr))
			{
				adapterFound = true;
				usedCard = desc.Description;
			}
			else
			{
				adapter.Reset();
				Log::Info("Device could not be created, trying WARP -ERROR:" + std::to_string(hr));
			}
		}
	}
	if (adapterFound) {
		Log::Info(L"Used graphic card: " + usedCard);
		return true;
	}
	ComPtr<IDXGIAdapter> warpAdapter;
	hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
	if (SUCCEEDED(hr)) {
		hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log::Info("Used WARP Adapter");
			return true;
		}
		else {
			warpAdapter.Reset();
			Log::Error("Error creating WARP device, terminating -ERROR:" + std::to_string(hr));
			return false;
		}
	}
	else {
		warpAdapter.Reset();
		Log::Error("Error finding WARP adapter -ERROR:" + std::to_string(hr));
		return false;
	}
}

bool DeviceManager::CreateComandQueue(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue>& commandQueue, ComPtr<ID3D12CommandAllocator>& commandAllocator, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Fence>& fence, HANDLE& fenceEvent, UINT64& fenceValue)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// Create command queue
	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr)) {
		Log::Error("Error creating command queue for device -ERROR:" + std::to_string(hr));
		return false;
	}

	// create command allocator
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	if (FAILED(hr)) {
		Log::Error("Error creating command allocator for device -ERROR:" + std::to_string(hr) );
		return false;
	}

	// create command list
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	if (FAILED(hr)) {
		Log::Error("Error creating command list for device -ERROR:" + std::to_string(hr));
		return false;
	}

	hr = commandList->Close();
	if (FAILED(hr)) {
		Log::Error("Error closing command list -ERROR:" + std::to_string(hr));
		return false;
	}

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	if (FAILED(hr)) {
		Log::Error("Error creating fence -ERROR:" + std::to_string(hr));
		return false;
	}
	fenceValue = 1;
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		if (FAILED(HRESULT_FROM_WIN32(GetLastError()))) {
			Log::Error("Error creating fence event");
			return false;
		}
	}
	return true;
}

bool DeviceManager::CreateSwapChain(HWND& hwnd, ComPtr<IDXGIFactory4>& factory, ComPtr<ID3D12CommandQueue>& commandQueue, UINT width, UINT height, ComPtr<IDXGISwapChain3>& swapChain, UINT& frameIndex)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = DXGE_FRAME_COUNT;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> tmpSwap;
	hr = factory->CreateSwapChainForHwnd(
		commandQueue.Get(),		// pointer to a direct command queue
		hwnd,					// The HWND handle that is associated with the swap chain that CreateSwapChainForHwnd creates.
								// This parameter cannot be NULL.
		&swapChainDesc,			// A pointer to a DXGI_SWAP_CHAIN_DESC1 structure for the swap-chain description.
								// This parameter cannot be NULL.
		nullptr,				// A pointer to a DXGI_SWAP_CHAIN_FULLSCREEN_DESC structure for the description of a full-screen swap chain
								// Set it to NULL to create a windowed swap chain.
		nullptr,				// A pointer to the IDXGIOutput interface for the output to restrict content to
								// NULL if you don't want to restrict content to an output target.
		&tmpSwap				// A pointer to a variable that receives a pointer to the IDXGISwapChain1 interface for the swap chain that CreateSwapChainForHwnd creates.
	);

	if (FAILED(hr)) {
		Log::Error("Error creating swap chain -ERROR:" + std::to_string(hr));
		return false;
	}

	// Restrict ALT+ENTER fullscreen mode
	hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		Log::Error("Error make window association alt enter -ERROR:" + std::to_string(hr));
		return false;
	}

	// Swap chain cast to higher swap chain functionality
	hr = tmpSwap.As(&swapChain);
	if (FAILED(hr)) {
		Log::Error("Error cast swap chain -ERROR:" + std::to_string(hr));
		return false;
	}
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	return true;
}

bool DeviceManager::CreateRenderTargets(ComPtr<ID3D12Device>& device, ComPtr<IDXGISwapChain3>& swapChain, ComPtr<ID3D12Resource> renderTargets[DXGE_FRAME_COUNT], ComPtr<ID3D12DescriptorHeap>& rtvHeap, UINT & rtvDescriptorSize)
{

	// Create render target view (RTV) heap descriptor
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = DXGE_FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	if (FAILED(hr)) {
		Log::Error("Error create descriptor heap -ERROR:" + std::to_string(hr));
		return false;
	}

	// Gets the size of the handle increment for the given type of descriptor heap.
	// This value is typically used to increment a handle into a descriptor array by the correct amount.
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create RTV for each frame
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT counter = 0; counter < DXGE_FRAME_COUNT; counter++) {
		hr = swapChain->GetBuffer(counter, IID_PPV_ARGS(&renderTargets[counter]));
		if (FAILED(hr)) {
			Log::Error("Error get swap chain buffer -ERROR:" + std::to_string(hr));
			return false;
		}
		device->CreateRenderTargetView(renderTargets[counter].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}
	return true;
}