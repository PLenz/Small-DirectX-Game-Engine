#include "stdafx.h"
#pragma once
#include "graphics.h"
#include "renderer.h"
#include "DeviceManager.h"
#include "TriangleRenderer.h"
#include "QuadRenderer.h"
#include "DynTriangleRenderer.h"
#include "DepthQuadRenderer.h"
#include "Camera.h"

Renderer* m_renderer;


bool Graphics::Init(HWND hwnd, int width, int height, int vertCount, Renderer* r) {
  if (!m_loaded) {
    //First time initialization
    m_renderer = r;

    //Call DeviceManager Methods to initialize Direct3D
    //In case of errors, use Error Logging and return false

    // Create Device
    if (!DeviceManager::CreateDevice(m_device, m_factory)) {
      Log::Error("Error on creating device");
      return false;
    }

    // Create Command Queue
    if (!DeviceManager::CreateComandQueue(m_device, m_commandQueue, m_commandAllocator, m_commandList, m_fence,
                                          m_fenceEvent, m_fenceValue)) {
      Log::Error("Error on creating command queue");
      return false;
    }

    // Create Swap Chain
    if (!DeviceManager::CreateSwapChain(hwnd, m_factory, m_commandQueue, width, height, m_swapChain, m_frameIndex)) {
      Log::Error("Error on creating swap chain");
      return false;
    }

    // Create Render Target
    if (!DeviceManager::CreateRenderTargets(m_device, m_swapChain, m_renderTargets, m_rtvHeap, m_rtvDescriptorSize)) {
      Log::Error("Error on creating render targets");
      return false;
    }

    // m_renderer = new Renderer();
    // m_renderer = new TriangleRenderer();
    // m_renderer = new DynTriangleRenderer(vertCount);
    // m_renderer = new QuadRenderer();
    // m_renderer = new DepthQuadRenderer();
    // m_renderer = new WorldRenderer();

    // Call Renderer Methods to CreatePipelineState and LoadResources
    // In case of errors, use Error Logging and return false
    if (!m_renderer->CreatePipelineState(m_device, width, height)) {
      Log::Error("Error on creating pipeline state");
      return false;
    }

    if (!m_renderer->LoadResources(m_device, m_commandList, m_commandAllocator, width, height)) {
      Log::Error("Error on load resources");
      return false;
    }

    Camera::Setup(m_renderer);

    // create an array of command lists (only one command list here)
    ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
    // execute the array of command lists
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


    // this command goes in at the end of our command queue. we will know when our command queue 
    // has finished because the fence value will be set to "fenceValue" from the GPU since the command
    // queue is being executed on the GPU
    const UINT64 fence = m_fenceValue;
    HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
    if (FAILED(hr)) {
      Log::Error("Error on commandQueue->Signal()");
      return false;
    }
    m_fenceValue++;
    Sync();
    m_loaded = true;

    return true;
  }
  return true;
  Log::Error("Initialization failed");
  return false;
}


bool Graphics::Render(double delta) {
  if (m_loaded) {
    // Call Update on the renderer
    m_renderer->Update(m_frameIndex, delta);
    // Get the CPU descriptor handle for RTV and call PopulateCommandList on the renderer
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex,
                                            m_rtvDescriptorSize);
    if (!m_renderer->PopulateCommandList(m_commandList, m_commandAllocator, rtvHandle, m_renderTargets[m_frameIndex],
                                         m_frameIndex)) {
      // In case of error, unload and return false
      Release();
      Log::Error("Error calling populate command list");
      return false;
    }
    // m_commandQueue->ExecuteCommandLists()
    ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // m_swapChain->Present(1, 0);
    // if (0,0) no syncronization, if (1,0) uses v-sync
    HRESULT hr = m_swapChain->Present(1, 0);

    // IF (FAILED) m_device->GetDeviceRemovedReason()
    if (FAILED(hr)) {
      Log::Error("Error presenting swap chain" + std::to_string(hr));
      hr = m_device->GetDeviceRemovedReason();
      Log::Error("Device Removed Reason: " + std::to_string(hr));
      return false;
    }
    return Sync();
  }
  return true;
}


bool Graphics::Sync() {
  // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
  // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
  // sample illustrates how to use fences for efficient resource usage and to
  // maximize GPU utilization.

  // Signal and increment the fence value.
  const UINT64 fence = m_fenceValue;
  HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
  if (FAILED(hr))
    return false;
  m_fenceValue++;

  // Wait until the previous frame is finished.
  if (m_fence->GetCompletedValue() < fence) {
    hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
    if (FAILED(hr))
      return false;
    WaitForSingleObject(m_fenceEvent, INFINITE);
  }

  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  return true;
}


void Graphics::Release() {
  m_loaded = false;

  //1: First wait for the GPU to finish all outstanding frames
  for (int i = 0; i < DXGE_FRAME_COUNT; i++) {
    m_frameIndex = i;
    this->Sync();
  }

  //2: then call Release on the Renderer
  m_renderer->Release();

  //3: then free all resources in this class
  m_factory.Reset();
  m_device.Reset();
  m_swapChain.Reset();
  m_commandQueue.Reset();
  m_rtvHeap.Reset();
  m_commandList.Reset();

  for (int i = 0; i < DXGE_FRAME_COUNT; i++) {
    m_renderTargets[i].Reset();
  }

  m_commandAllocator.Reset();
  m_fence.Reset();
  m_swapChain.Reset();
}
