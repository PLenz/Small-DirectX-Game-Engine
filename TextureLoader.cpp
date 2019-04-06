#include "stdafx.h"
#include "TextureLoader.h"


int TextureLoader::LoadImageDataFromFile(BYTE** pp_image_data, D3D12_RESOURCE_DESC& resource_description, LPCWSTR filename,
                                         int& bytes_per_row) {
  HRESULT hr;

  // reset decoder, frame and converter since these will be different for each image we load
  IWICBitmapDecoder* wicDecoder = nullptr;
  IWICBitmapFrameDecode* wicFrame = nullptr;
  IWICFormatConverter* wicConverter = nullptr;

  bool imageConverted = false;
  static IWICImagingFactory* m_wicFactory;
  if (m_wicFactory == nullptr) {
    // Initialize the COM library
    CoInitialize(nullptr);

    // create the WIC factory
    hr = CoCreateInstance(
      CLSID_WICImagingFactory,
      nullptr,
      CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(&m_wicFactory)
    );
    if (FAILED(hr))
      return 0;
  }

  // load a decoder for the image
  hr = m_wicFactory->CreateDecoderFromFilename(
    filename, // Image we want to load in
    nullptr, // This is a vendor ID, we do not prefer a specific one so set to null
    GENERIC_READ, // We want to read from this file
    WICDecodeMetadataCacheOnLoad,
    // We will cache the metadata right away, rather than when needed, which might be unknown
    &wicDecoder // the wic decoder to be created
  );
  if (FAILED(hr))
    return 0;

  // get image from decoder (this will decode the "frame")
  hr = wicDecoder->GetFrame(0, &wicFrame);
  if (FAILED(hr))
    return 0;

  // get wic pixel format of image
  WICPixelFormatGUID pixelFormat;
  hr = wicFrame->GetPixelFormat(&pixelFormat);
  if (FAILED(hr))
    return 0;

  // get size of image
  UINT textureWidth, textureHeight;
  hr = wicFrame->GetSize(&textureWidth, &textureHeight);
  if (FAILED(hr))
    return 0;

  // we are not handling sRGB types in this tutorial, so if you need that support, you'll have to figure
  // out how to implement the support yourself

  // convert wic pixel format to dxgi pixel format
  DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

  // if the format of the image is not a supported dxgi format, try to convert it
  if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
    // get a dxgi compatible wic format from the current image format
    WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

    // return if no dxgi compatible format was found
    if (convertToPixelFormat == GUID_WICPixelFormatDontCare)
      return 0;

    // set the dxgi format
    dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

    // create the format converter
    hr = m_wicFactory->CreateFormatConverter(&wicConverter);
    if (FAILED(hr))
      return 0;

    // make sure we can convert to the dxgi compatible format
    BOOL canConvert = FALSE;
    hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
    if (FAILED(hr) || !canConvert)
      return 0;

    // do the conversion (wicConverter will contain the converted image)
    hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, nullptr, 0,
                                  WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
      return 0;

    // this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
    imageConverted = true;
  }

  int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
  bytes_per_row = (textureWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
  int imageSize = bytes_per_row * textureHeight; // total image size in bytes

  // allocate enough memory for the raw image data, and set pp_image_data to point to that memory
  *pp_image_data = (BYTE*)malloc(imageSize);

  // copy (decoded) raw image data into the newly allocated memory (pp_image_data)
  if (imageConverted) {
    // if image format needed to be converted, the wic converter will contain the converted image
    hr = wicConverter->CopyPixels(nullptr, bytes_per_row, imageSize, *pp_image_data);
    if (FAILED(hr))
      return 0;
  } else {
    // no need to convert, just copy data from the wic frame
    hr = wicFrame->CopyPixels(nullptr, bytes_per_row, imageSize, *pp_image_data);
    if (FAILED(hr))
      return 0;
  }

  // now describe the texture with the information we have obtained from the image
  resource_description = {};
  resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resource_description.Alignment = 0;
  // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
  resource_description.Width = textureWidth; // width of the texture
  resource_description.Height = textureHeight; // height of the texture
  resource_description.DepthOrArraySize = 1;
  // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
  resource_description.MipLevels = 0;
  // Number of mipmaps. We are not generating default mipmaps for this texture, so 0
  resource_description.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
  resource_description.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
  resource_description.SampleDesc.Quality = 0;
  // The quality level of the samples. Higher is better quality, but worse performance
  resource_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
  resource_description.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

  // return the size of the image. remember to delete the image once your done with it (in this tutorial once its uploaded to the gpu)
  return imageSize;
}


// get the dxgi format equivilent of a wic format
DXGI_FORMAT TextureLoader::GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wic_format_guid) {
  if (wic_format_guid == GUID_WICPixelFormat128bppRGBAFloat)
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGBAHalf)
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGBA)
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppRGBA)
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppBGRA)
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppBGR)
    return DXGI_FORMAT_B8G8R8X8_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppRGBA1010102XR)
    return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppRGBA1010102)
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat16bppBGRA5551)
    return DXGI_FORMAT_B5G5R5A1_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat16bppBGR565)
    return DXGI_FORMAT_B5G6R5_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat32bppGrayFloat)
    return DXGI_FORMAT_R32_FLOAT;
  if (wic_format_guid == GUID_WICPixelFormat16bppGrayHalf)
    return DXGI_FORMAT_R16_FLOAT;
  if (wic_format_guid == GUID_WICPixelFormat16bppGray)
    return DXGI_FORMAT_R16_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat8bppGray)
    return DXGI_FORMAT_R8_UNORM;
  if (wic_format_guid == GUID_WICPixelFormat8bppAlpha)
    return DXGI_FORMAT_A8_UNORM;
  return DXGI_FORMAT_UNKNOWN;

}


// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID TextureLoader::GetConvertToWICFormat(WICPixelFormatGUID& wic_format_guid) {
  if (wic_format_guid == GUID_WICPixelFormatBlackWhite)
    return GUID_WICPixelFormat8bppGray;
  if (wic_format_guid == GUID_WICPixelFormat1bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat2bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat4bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat8bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat2bppGray)
    return GUID_WICPixelFormat8bppGray;
  if (wic_format_guid == GUID_WICPixelFormat4bppGray)
    return GUID_WICPixelFormat8bppGray;
  if (wic_format_guid == GUID_WICPixelFormat16bppGrayFixedPoint)
    return GUID_WICPixelFormat16bppGrayHalf;
  if (wic_format_guid == GUID_WICPixelFormat32bppGrayFixedPoint)
    return GUID_WICPixelFormat32bppGrayFloat;
  if (wic_format_guid == GUID_WICPixelFormat16bppBGR555)
    return GUID_WICPixelFormat16bppBGRA5551;
  if (wic_format_guid == GUID_WICPixelFormat32bppBGR101010)
    return GUID_WICPixelFormat32bppRGBA1010102;
  if (wic_format_guid == GUID_WICPixelFormat24bppBGR)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat24bppRGB)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat32bppPBGRA)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat32bppPRGBA)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat48bppRGB)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat48bppBGR)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppBGRA)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppPRGBA)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppPBGRA)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat48bppRGBFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat48bppBGRFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGBAFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat64bppBGRAFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGBFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGBHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat48bppRGBHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
  if (wic_format_guid == GUID_WICPixelFormat128bppPRGBAFloat)
    return GUID_WICPixelFormat128bppRGBAFloat;
  if (wic_format_guid == GUID_WICPixelFormat128bppRGBFloat)
    return GUID_WICPixelFormat128bppRGBAFloat;
  if (wic_format_guid == GUID_WICPixelFormat128bppRGBAFixedPoint)
    return GUID_WICPixelFormat128bppRGBAFloat;
  if (wic_format_guid == GUID_WICPixelFormat128bppRGBFixedPoint)
    return GUID_WICPixelFormat128bppRGBAFloat;
  if (wic_format_guid == GUID_WICPixelFormat32bppRGBE)
    return GUID_WICPixelFormat128bppRGBAFloat;
  if (wic_format_guid == GUID_WICPixelFormat32bppCMYK)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppCMYK)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat40bppCMYKAlpha)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat80bppCMYKAlpha)
    return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
  if (wic_format_guid == GUID_WICPixelFormat32bppRGB)
    return GUID_WICPixelFormat32bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppRGB)
    return GUID_WICPixelFormat64bppRGBA;
  if (wic_format_guid == GUID_WICPixelFormat64bppPRGBAHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
#endif
  return GUID_WICPixelFormatDontCare;
}


// get the number of bits per pixel for a dxgi format
int TextureLoader::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgi_format) {
  if (dxgi_format == DXGI_FORMAT_R32G32B32A32_FLOAT)
    return 128;
  if (dxgi_format == DXGI_FORMAT_R16G16B16A16_FLOAT)
    return 64;
  if (dxgi_format == DXGI_FORMAT_R16G16B16A16_UNORM)
    return 64;
  if (dxgi_format == DXGI_FORMAT_R8G8B8A8_UNORM)
    return 32;
  if (dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM)
    return 32;
  if (dxgi_format == DXGI_FORMAT_B8G8R8X8_UNORM)
    return 32;
  if (dxgi_format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
    return 32;
  if (dxgi_format == DXGI_FORMAT_R10G10B10A2_UNORM)
    return 32;
  if (dxgi_format == DXGI_FORMAT_B5G5R5A1_UNORM)
    return 16;
  if (dxgi_format == DXGI_FORMAT_B5G6R5_UNORM)
    return 16;
  else {
    if (dxgi_format == DXGI_FORMAT_R32_FLOAT)
      return 32;
    if (dxgi_format == DXGI_FORMAT_R16_FLOAT)
      return 16;
    if (dxgi_format == DXGI_FORMAT_R16_UNORM)
      return 16;
    if (dxgi_format == DXGI_FORMAT_R8_UNORM)
      return 8;
    if (dxgi_format == DXGI_FORMAT_A8_UNORM)
      return 8;
  }

}
