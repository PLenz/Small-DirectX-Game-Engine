#include "stdafx.h"
#pragma once
#include "graphics.h"
#include "Renderer.h"
#include "DeviceManager.h"
#include "TriangleRenderer.h"
#include "QuadRenderer.h"
#include "DynTriangleRenderer.h"
#include "DepthQuadRenderer.h"
#include "Camera.h"

Renderer* m_renderer;


bool Graphics::Init(HWND hwnd, int width, int height, int vert_count, Renderer* renderer) {
  if (!m_loaded_) {
    //First time initialization
    p_renderer_ = renderer;

    //Call DeviceManager Methods to initialize Direct3D
    //In case of errors, use Error Logging and return false

    // Create Device
    if (!DeviceManager::CreateDevice(m_device_, m_factory_)) {
      Log::Error("Error on creating device");
      return false;
    }

    // Create Command Queue
    if (!DeviceManager::CreateComandQueue(m_device_, m_command_queue_, m_command_allocator_, m_command_list_, m_fence_,
                                            m_fence_event_, m_fence_value_)) {
      Log::Error("Error on creating command queue");
      return false;
    }

    // Create Swap Chain
    if (!DeviceManager::CreateSwapChain(hwnd, m_factory_, m_command_queue_, width, height, m_swap_chain_, m_frame_index_)) {
      Log::Error("Error on creating swap chain");
      return false;
    }

    // Create Render Target
    if (!DeviceManager::CreateRenderTargets(m_device_, m_swap_chain_, m_render_targets_, m_rtv_heap_, m_rtv_descriptor_size_)) {
      Log::Error("Error on creating render targets");
      return false;
    }

    // p_renderer_ = new Renderer();
    // p_renderer_ = new TriangleRenderer();
    // p_renderer_ = new DynTriangleRenderer(vert_count);
    // p_renderer_ = new QuadRenderer();
    // p_renderer_ = new DepthQuadRenderer();
    // p_renderer_ = new WorldRenderer();

    // Call Renderer Methods to CreatePipelineState and LoadResources
    // In case of errors, use Error Logging and return false
    if (!p_renderer_->CreatePipelineState(m_device_, width, height)) {
      Log::Error("Error on creating pipeline state");
      return false;
    }

    if (!p_renderer_->LoadResources(m_device_, m_command_list_, m_command_allocator_, width, height)) {
      Log::Error("Error on load resources");
      return false;
    }

    Camera::Setup(p_renderer_);

    // create an array of command lists (only one command list here)
    ID3D12CommandList* ppCommandLists[] = {m_command_list_.Get()};
    // execute the array of command lists
    m_command_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


    // this command goes in at the end of our command queue. we will know when our command queue 
    // has finished because the fence value will be set to "fenceValue" from the GPU since the command
    // queue is being executed on the GPU
    const UINT64 fence = m_fence_value_;
    HRESULT hr = m_command_queue_->Signal(m_fence_.Get(), fence);
    if (FAILED(hr)) {
      Log::Error("Error on commandQueue->Signal()");
      return false;
    }
    m_fence_value_++;
    Sync();
    m_loaded_ = true;

    return true;
  }
  return true;
  Log::Error("Initialization failed");
  return false;
}


bool Graphics::Render(double delta) {
  if (m_loaded_) {
    // Call Update on the renderer
    p_renderer_->Update(m_frame_index_, delta);
    // Get the CPU descriptor handle for RTV and call PopulateCommandList on the renderer
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(), m_frame_index_,
                                            m_rtv_descriptor_size_);
    if (!p_renderer_->PopulateCommandList(m_command_list_, m_command_allocator_, rtvHandle, m_render_targets_[m_frame_index_],
                                           m_frame_index_)) {
      // In case of error, unload and return false
      Release();
      Log::Error("Error calling populate command list");
      return false;
    }
    // m_command_queue_->ExecuteCommandLists()
    ID3D12CommandList* ppCommandLists[] = {m_command_list_.Get()};
    m_command_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // m_swap_chain_->Present(1, 0);
    // if (0,0) no syncronization, if (1,0) uses v-sync
    HRESULT hr = m_swap_chain_->Present(1, 0);

    // IF (FAILED) m_device_->GetDeviceRemovedReason()
    if (FAILED(hr)) {
      Log::Error("Error presenting swap chain" + std::to_string(hr));
      hr = m_device_->GetDeviceRemovedReason();
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
  const UINT64 fence = m_fence_value_;
  HRESULT hr = m_command_queue_->Signal(m_fence_.Get(), fence);
  if (FAILED(hr))
    return false;
  m_fence_value_++;

  // Wait until the previous frame is finished.
  if (m_fence_->GetCompletedValue() < fence) {
    hr = m_fence_->SetEventOnCompletion(fence, m_fence_event_);
    if (FAILED(hr))
      return false;
    WaitForSingleObject(m_fence_event_, INFINITE);
  }

  m_frame_index_ = m_swap_chain_->GetCurrentBackBufferIndex();
  return true;
}


void Graphics::Release() {
  m_loaded_ = false;

  //1: First wait for the GPU to finish all outstanding frames
  for (int i = 0; i < DXGE_FRAME_COUNT; i++) {
    m_frame_index_ = i;
    this->Sync();
  }

  //2: then call Release on the Renderer
  p_renderer_->Release();

  //3: then free all resources in this class
  m_factory_.Reset();
  m_device_.Reset();
  m_swap_chain_.Reset();
  m_command_queue_.Reset();
  m_rtv_heap_.Reset();
  m_command_list_.Reset();

  for (int i = 0; i < DXGE_FRAME_COUNT; i++) {
    m_render_targets_[i].Reset();
  }

  m_command_allocator_.Reset();
  m_fence_.Reset();
  m_swap_chain_.Reset();
}
