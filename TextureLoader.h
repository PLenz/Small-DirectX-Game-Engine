#pragma once
#include "stdafx.h"

class TextureLoader {
public:
  static int LoadImageDataFromFile(BYTE** pp_image_data, D3D12_RESOURCE_DESC& resource_description, LPCWSTR filename,
                                   int& bytes_per_row);
private:
  TextureLoader() { };


  ~TextureLoader() { };


  static DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wic_format_guid);
  static WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wic_format_guid);
  static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgi_format);
};
