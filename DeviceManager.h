#pragma once
class DeviceManager {
public:
  static bool CreateDevice(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory);
  static bool CreateComandQueue(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue>& command_queue,
                                  ComPtr<ID3D12CommandAllocator>& command_allocator,
                                  ComPtr<ID3D12GraphicsCommandList>& command_list, ComPtr<ID3D12Fence>& fence,
                                  HANDLE& fence_event, UINT64& fence_value);
  static bool CreateSwapChain(HWND& hwnd, ComPtr<IDXGIFactory4>& factory, ComPtr<ID3D12CommandQueue>& command_queue,
                                UINT width, UINT height, ComPtr<IDXGISwapChain3>& swap_chain, UINT& frame_index);
  static bool CreateRenderTargets(ComPtr<ID3D12Device>& device, ComPtr<IDXGISwapChain3>& swap_chain,
                                    ComPtr<ID3D12Resource> render_targets[DXGE_FRAME_COUNT],
                                    ComPtr<ID3D12DescriptorHeap>& rtv_heap, UINT& rtv_descriptor_size);

private:
  DeviceManager() {};


  ~DeviceManager() {};
};
