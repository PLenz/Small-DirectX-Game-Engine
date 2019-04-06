#pragma once
#include "stdafx.h"
#include "Renderer.h"

class Graphics {
public:
  Graphics() {};


  ~Graphics() {};


  bool Init(HWND, int window_width, int window_height, int vert_count, Renderer* renderer);
  bool Render(double delta);
  void Release();


  Renderer GetRenderer() {
    return *p_renderer_;
  };


private:
  bool Sync();

  bool m_loaded_ = false;
  ComPtr<ID3D12Device> m_device_;
  ComPtr<IDXGISwapChain3> m_swap_chain_;
  ComPtr<ID3D12Resource> m_render_targets_[DXGE_FRAME_COUNT];

  ComPtr<ID3D12CommandQueue> m_command_queue_;
  ComPtr<ID3D12CommandAllocator> m_command_allocator_;
  ComPtr<ID3D12GraphicsCommandList> m_command_list_;
  HANDLE m_fence_event_;
  ComPtr<ID3D12Fence> m_fence_;
  UINT64 m_fence_value_;

  ComPtr<IDXGIFactory4> m_factory_;
  ComPtr<ID3D12DescriptorHeap> m_rtv_heap_;
  UINT m_rtv_descriptor_size_;
  UINT m_frame_index_;

  Renderer* p_renderer_;
};
