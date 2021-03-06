#include "stdafx.h"
#pragma once
#include "Main.h"
#include "graphics.h"
#include "Camera.h"
#include "Keyboard.h"
#include "ImageRenderer.h"
#include "WorldRenderer.h"
#include "DynTriangleRenderer.h"

HWND hwnd = nullptr;
LPCTSTR WindowName = L"DxWindow";
LPCTSTR WindowTitle = L"DX Window";
RECT currentWinRect;
bool fullScreen = false;
float fps = 0.0;

void mainloop();
void wm_input(WPARAM, LPARAM);
void wm_keydown(WPARAM, LPARAM);
void toggleFullscreen();
void updateWin();
void setNewWinSize();
void Benchmark(int);
bool loadSplashscreen();

Graphics G;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hprevinst, LPSTR cmdline, int ncmdshow) {
  RAWINPUTDEVICE Rid[1];
  ZeroMemory(&Rid, sizeof(RAWINPUTDEVICE));

  Rid[0].usUsagePage = 0x01;
  Rid[0].usUsage = 0x02; // adds mouse

  if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
    //registration failed. Call GetLastError for the cause of the error.
    Log::Error(L"Register Raw input - Failed");
    return 0;
  }

  hwnd = InitWindow(hInstance, WindowTitle, DXGE_WINDOW_DEFAULT_WIDTH, DXGE_WINDOW_DEFAULT_HEIGHT, fullScreen, 0);

  if (!hwnd) {
    Log::Error(L"Window Initialization - Failed");
    return 0;
  }

  double PCFreq = 0;
  __int64 CounterStart = 0;
  double timeInMs = 0.0;
  LARGE_INTEGER li;
  if (!QueryPerformanceFrequency(&li)) {
    Log::Error("QueryPerformance failed");
    return 0;
  }
  PCFreq = double(li.QuadPart) / 1000;

  // take startTime
  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;

  loadSplashscreen();

  // init Graphics with world renderer here
  if (G.Init(hwnd, DXGE_WINDOW_DEFAULT_WIDTH, DXGE_WINDOW_DEFAULT_HEIGHT, m_vertCount, new WorldRenderer())) {
    QueryPerformanceCounter(&li);
    timeInMs = double(li.QuadPart - CounterStart) / PCFreq;

    if (timeInMs < 2000) {
      Sleep(2000 - timeInMs);
    }
    Camera::SetCameraPosition(XMFLOAT3(10.0f, 1.8f, 14.0f));
    mainloop();
  }
  return 0;
}


void mainloop() {
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));

  // Message Loop until window quits
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // run game code
      double delta = m_timer.GetFrameDelta();
      if (!G.Render(delta)) {
        // End programm on error
        break;
      }
      m_splashscreen = false;
    }
  }
  G.Release();
}


bool loadSplashscreen() {
  m_splashscreen = true;
  while (ShowCursor(false) == 0);
  if (!G.Init(hwnd, DXGE_WINDOW_DEFAULT_WIDTH, DXGE_WINDOW_DEFAULT_HEIGHT, m_vertCount, new ImageRenderer())) {
    Log::Error("Init m_splashscreen failed");
    return false;
  }
  Camera::SetCameraPosition(XMFLOAT3(0.0f, 0.0f, 2.0f));

  double delta = m_timer.GetFrameDelta();
  if (!G.Render(delta)) {
    // End programm on error
    return false;
  }

  G.Release();
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INPUT:
      if (!m_splashscreen) {
        wm_input(wParam, lParam);
      }
      break;
    case WM_KEYDOWN:
      if (!m_splashscreen) {
        // Register keydown actions
        Keyboard::KeyDown(wParam);
        wm_keydown(wParam, lParam);
      }
      break;
    case WM_KEYUP:
      Keyboard::KeyUp(wParam);
      break;
    case WM_DESTROY:
      // Destroy the window
      PostQuitMessage(0);
      return 0;
    case WM_MOUSEMOVE:
      // Hide the cursor inside the window
      while (ShowCursor(false) == 0);
      break;
    default:
      break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}


void wm_input(WPARAM wParam, LPARAM lParam) {
  HRESULT hr;
  UINT dwSize;
  GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
  LPBYTE lpb = new BYTE[dwSize];
  if (lpb == nullptr) {
    Log::Error(L"lpb returns null - Failed");
    return;
  }

  if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
    Log::Error("GetRawInputData does not return correct size !");
    return;
  }

  RAWINPUT* raw = (RAWINPUT*)lpb;

  if (raw->header.dwType == RIM_TYPEMOUSE) {
    //lLastY/Y = signed relative or absolute motion in the X/Y direction
    Camera::Rotate(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
  }
  delete[] lpb;
}


void wm_keydown(WPARAM wParam, LPARAM lParam) {
  switch (wParam) {
    case VK_ESCAPE:
      DestroyWindow(hwnd);
      break;
    case VK_F11:
      // toggle fullscreen
      toggleFullscreen();

      // Set the new window size
      setNewWinSize();

      // Resize and init with new width/height
      updateWin();
      break;
    case 0x48:
      // 0x48 = H-Key
      Log::Info("");
      Log::Info("#############################");
      Log::Info("#			HELP			#");
      Log::Info("#___________________________#");
      Log::Info("#							#");
      Log::Info("#    W: Move forward		#");
      Log::Info("#    A: Move left			#");
      Log::Info("#    S: Move backward		#");
      Log::Info("#    D: Move right			#");
      Log::Info("#    H: Show this help		#");
      Log::Info("#    I: Show current fps	#");
      Log::Info("#  F11: Toggle fullscreen	#");
      Log::Info("#  ESC: Close the programm	#");
      Log::Info("#							#");
      Log::Info("#############################");
      Log::Info("");
      break;
    case 0x49:
      // 0x48 = I-Key
      Log::Info("FPS: " + std::to_string(m_timer.fps));
      break;
    default:
      break;
  }
}


void toggleFullscreen() {
  WINDOWPLACEMENT wndpl;
  GetWindowPlacement(hwnd, &wndpl);
  // Maximize the window
  wndpl.showCmd = wndpl.showCmd == SW_SHOWMAXIMIZED ? SW_SHOWNORMAL : SW_SHOWMAXIMIZED;
  SetWindowPlacement(hwnd, &wndpl);
}


void updateWin() {
  G.Release();
  G.Init(hwnd, m_width, m_height, m_vertCount, new WorldRenderer());
}


void setNewWinSize() {
  GetWindowRect(hwnd, &currentWinRect);
  m_width = currentWinRect.right - currentWinRect.left;
  m_height = currentWinRect.bottom - currentWinRect.top;
}


HWND InitWindow(HINSTANCE h_instance, LPCWSTR window_title, UINT width, UINT height, bool full_screen,
                int background_color) {
  HWND hwnd = nullptr;

  // RECT is used to determine the window position
  RECT rc;
  GetClientRect(GetDesktopWindow(), &rc);

  WNDCLASS wc;

  ZeroMemory(&wc, sizeof(WNDCLASS));

  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = NULL;
  wc.cbWndExtra = NULL;
  wc.hInstance = h_instance;
  wc.hIcon = LoadIcon(h_instance, MAKEINTRESOURCE(IDI_ICON_THM));
  wc.hCursor = nullptr;
  wc.hbrBackground = CreateSolidBrush(background_color);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = WindowName;

  if (!RegisterClass(&wc)) {
    Log::Error(L"Error registering class");
    return nullptr;
  }

  hwnd =  CreateWindow(
		wc.lpszClassName,
		window_title,
		WS_POPUPWINDOW,
		// set rc left and top to center window
		(rc.right - width) * 0.5,
		(rc.bottom - height) * 0.5,
		width, height,
		NULL, // handle to the parent or owner window of the window being created
		NULL, // handle to a menu, or specifies a child-window identifier depending on the window style
		h_instance,
		NULL // A pointer to a value to be passed to the window through the CREATESTRUCT structure (lpCreateParams member)
			//	pointed to by the lParam param of the WM_CREATE message. 
			// This message is sent to the created window by this function before it returns.
			// lpParam may be NULL if no additional data is needed
	);

  if (!hwnd) {
    Log::Error(L"Error creating window");
    return nullptr;
  }

  ShowWindow(hwnd, full_screen ? SHOW_FULLSCREEN : SHOW_OPENWINDOW);
  UpdateWindow(hwnd);
  return hwnd;
}


void Benchmark(int type) {
  float x = 0.0;
  float y = 0.0;
  float z = 0.0;
  float v = 13;
  float t = 13;
  double PCFreq = 0;
  __int64 CounterStart = 0;
  double timeInMs = 0.0;
  LARGE_INTEGER li;
  XMFLOAT4X4 target = {};
  XMFLOAT4 vectorTarget = {};
  if (!QueryPerformanceFrequency(&li)) {
    Log::Error("QueryPerformance failed");
    return;
  }

  PCFreq = double(li.QuadPart) / 1000;

  // take startTime
  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;

  if (type == 1) {
    for (int i = 0; i < 10000; ++i) {
      x += v * t * i;
      y += v * t * (i + 1);
      z += v * t * (i + 2);
    }
  } else if (type == 2) {
    for (int i = 0; i < 10000; ++i) {
      x += v * t * i;
      y += v * t * (i + 1);
      z += v * t * (i + 2);
      XMFLOAT3 coord = {x, y, z};
      XMMATRIX matrix = XMMatrixRotationAxis(XMLoadFloat3(&coord), i);
      XMStoreFloat4x4(&target, matrix);
    }
  } else if (type == 3) {
    for (int i = 0; i < 10000; ++i) {
      x += v * t * i;
      y += v * t * (i + 1);
      z += v * t * (i + 2);
      XMMATRIX matrix = XMMatrixRotationRollPitchYaw(x, y, z);
      XMStoreFloat4x4(&target, matrix);
    }
  } else if (type == 4) {
    for (int i = 0; i < 10000; ++i) {
      x += v * t * i;
      y += v * t * (i + 1);
      z += v * t * (i + 2);
      XMVECTOR vector = XMQuaternionRotationRollPitchYaw(x, y, z);
      XMStoreFloat4(&vectorTarget, vector);
    }
  }

  QueryPerformanceCounter(&li);
  timeInMs = double(li.QuadPart - CounterStart) / PCFreq;
  //take endTime
  Log::Info("Benchmark type " + std::to_string(type) + " finished in " + std::to_string(timeInMs) + "ms");
}
