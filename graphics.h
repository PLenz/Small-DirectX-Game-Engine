#pragma once
#include "stdafx.h"
#include "renderer.h"

class Graphics {
public:
  Graphics() {};


  ~Graphics() {};


  bool Init(HWND, int windowWidth, int windowHeight, int vertCount, Renderer* r);
  bool Render(double delta);
  void Release();


  Renderer GetRenderer() {
    return *m_renderer;
  };


private:
  bool Sync();

  bool m_loaded = false;
  ComPtr<ID3D12Device> m_device;
  ComPtr<IDXGISwapChain3> m_swapChain;
  ComPtr<ID3D12Resource> m_renderTargets[DXGE_FRAME_COUNT];

  ComPtr<ID3D12CommandQueue> m_commandQueue;
  ComPtr<ID3D12CommandAllocator> m_commandAllocator;
  ComPtr<ID3D12GraphicsCommandList> m_commandList;
  HANDLE m_fenceEvent;
  ComPtr<ID3D12Fence> m_fence;
  UINT64 m_fenceValue;

  ComPtr<IDXGIFactory4> m_factory;
  ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
  UINT m_rtvDescriptorSize;
  UINT m_frameIndex;

  Renderer* m_renderer;
};
