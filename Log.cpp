#include "stdafx.h"
#include <iostream>


void Log::InfoVector(XMVECTOR& vec, std::string name) {
  XMFLOAT4 f;
  XMStoreFloat4(&f, vec);
  Info("Vector \"" + name + "\":");
  Info("x = " + STR(f.x));
  Info("y = " + STR(f.y));
  Info("z = " + STR(f.z));
  Info("w = " + STR(f.w));
}


void Log::Info(std::string message) {
  Info(std::wstring(message.begin(), message.end()));
}


void Log::Info(std::wstring message) {
  // Output to console
  OutputDebugString(message.c_str());
  OutputDebugString(L"\n");
}


void Log::Error(std::string message) {
  Error(std::wstring(message.begin(), message.end()));
}


void Log::Error(std::wstring message) {
  // output to message box
  OutputDebugString(message.c_str());
  OutputDebugString(L"\n");
  MessageBox(
    nullptr,
    message.c_str(),
    L"ERROR",
    MB_OK | MB_ICONERROR
  );
}
