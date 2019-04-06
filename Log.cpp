#include "stdafx.h"
#include <iostream>

void Log::InfoVector(XMVECTOR &vec, std::string name) {
	XMFLOAT4 f;
	XMStoreFloat4(&f, vec);
	Log::Info("Vector \"" + name + "\":");
	Log::Info("x = " + STR(f.x));
	Log::Info("y = " + STR(f.y));
	Log::Info("z = " + STR(f.z));
	Log::Info("w = " + STR(f.w));
}

void Log::Info(std::string message)
{
	Log::Info(std::wstring(message.begin(), message.end()));
}

void Log::Info(std::wstring message)
{
	// Output to console
	OutputDebugString(message.c_str());
	OutputDebugString(L"\n");
}

void Log::Error(std::string message)
{
	Log::Error(std::wstring(message.begin(), message.end()));
}

void Log::Error(std::wstring message)
{
	// output to message box
	OutputDebugString(message.c_str());
	OutputDebugString(L"\n");
	MessageBox(
		NULL,
		message.c_str(),
		L"ERROR",
		MB_OK | MB_ICONERROR
	);
}