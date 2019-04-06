#pragma once
class DeviceManager
{
public:
	static bool CreateDevice(ComPtr<ID3D12Device> &device, ComPtr<IDXGIFactory4> &factory);
	static bool CreateComandQueue(ComPtr<ID3D12Device> &device, ComPtr<ID3D12CommandQueue> &commandQueue, ComPtr<ID3D12CommandAllocator> &commandAllocator, ComPtr<ID3D12GraphicsCommandList> &commandList, ComPtr<ID3D12Fence> &fence, HANDLE &fenceEvent, UINT64 &fenceValue);
	static bool CreateSwapChain(HWND &hwnd, ComPtr<IDXGIFactory4> &factory, ComPtr<ID3D12CommandQueue> &commandQueue, UINT width, UINT height, ComPtr<IDXGISwapChain3> &swapChain, UINT &frameIndex);
	static bool CreateRenderTargets(ComPtr<ID3D12Device> &device, ComPtr<IDXGISwapChain3> &swapChain, ComPtr<ID3D12Resource> renderTargets[DXGE_FRAME_COUNT], ComPtr<ID3D12DescriptorHeap> &rtvHeap, UINT &rtvDescriptorSize);

private:
	DeviceManager() {};
	~DeviceManager() {};
};