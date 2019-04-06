#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

#define DXGE_FRAME_COUNT 2
#define DXGE_WINDOW_DEFAULT_WIDTH 1280
#define DXGE_WINDOW_DEFAULT_HEIGHT 720

#include "windows.h"
#include "resource.h"
#include <string>
#include <cstring>
#include <vector>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <initguid.h>
#include <dxgidebug.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <directxcollision.h>
#include <directxpackedvector.h>

#include "d3dx12.h"
#include <wincodec.h>

#include <wrl.h>
#include <shellapi.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"windowscodecs.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

#include "Log.h"

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
	bool operator==(const Vertex& r)
	{
		if (position.x != r.position.x)
		{
			return false;
		}
		if (position.y != r.position.y)
		{
			return false;
		}
		if (position.z != r.position.z)
		{
			return false;
		}

		if (color.w != r.color.w)
		{
			return false;
		}
		if (color.x != r.color.x)
		{
			return false;
		}
		if (color.y != r.color.y)
		{
			return false;
		}
		if (color.z != r.color.z)
		{
			return false;
		}

		if (texcoord.y != r.texcoord.y)
		{
			return false;
		}
		if (texcoord.x != r.texcoord.x)
		{
			return false;
		}

		return true;
	}

};

struct Triangle
{
	Vertex positions[3];
};

struct ConstantBuffer {
	XMFLOAT4 factor;
	XMFLOAT4X4 wvpMat;
};

#define THM_GREEN { 0.502f, 0.729f, 0.141f, 1.0f }
#define THM_GREY { 0.29f, 0.361f, 0.4f, 1.0f}
#define THM_RED { 0.612f, 0.075f, 0.177f, 1.0f}
#define WHITE {1.0f, 1.0f, 1.0f, 1.0f}

#define STR(x) std::to_string( x )