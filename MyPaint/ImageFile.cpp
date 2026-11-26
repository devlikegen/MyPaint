/**
 * @file ImageFile.cpp
 * @brief 이미지 파일 저장 구현
 */
#include "pch.h"
#include "ImageFile.h"
#include "CanvasSurface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    /**
     * @brief 불러올 수 있는 그림의 최대 한 변 길이(px).
     * 그림 하나가 픽셀당 4바이트이고, 그릴 때마다 되돌리기용 복사본을 하나 더 만들므로
     * 너무 큰 그림은 메모리를 다 쓰기 전에 거절합니다.
     */
    constexpr UINT kMaxImageSide = 16384;
    constexpr unsigned long long kMaxImagePixels = 64ull * 1024 * 1024; ///< 최대 픽셀 수 (64M px = 256MB)
}

bool ImageFormatFromPath(const CString& path, ImageFormat& format)
{
    const CString ext(::PathFindExtension(path));
    if (ext.CompareNoCase(L".png") == 0)
    {
        format = ImageFormat::Png;
        return true;
    }
    if (ext.CompareNoCase(L".bmp") == 0)
    {
        format = ImageFormat::Bmp;
        return true;
    }
    if (ext.CompareNoCase(L".jpg") == 0 || ext.CompareNoCase(L".jpeg") == 0)
    {
        format = ImageFormat::Jpeg;
        return true;
    }
    return false;
}

bool SaveImageFile(CCanvasSurface& surface, const CString& path, CString& errorMessage)
{
    ImageFormat format;
    if (!ImageFormatFromPath(path, format))
    {
        errorMessage = L"지원하지 않는 파일 형식입니다. PNG, BMP, JPEG 중 하나로 저장해 주세요.";
        return false;
    }
    if (!surface.IsValid())
    {
        errorMessage = L"저장할 그림이 없습니다.";
        return false;
    }

    const CSize size = surface.GetSize();
    CImage image;
    if (!image.Create(size.cx, size.cy, 24))
    {
        errorMessage = L"저장할 이미지를 만들지 못했습니다. 메모리가 부족할 수 있습니다.";
        return false;
    }
    {
        HDC hdc = image.GetDC();
        ::BitBlt(hdc, 0, 0, size.cx, size.cy, surface.GetDC().GetSafeHdc(), 0, 0, SRCCOPY);
        image.ReleaseDC();
    }

    GUID fileType = Gdiplus::ImageFormatPNG;
    switch (format)
    {
    case ImageFormat::Png:  fileType = Gdiplus::ImageFormatPNG; break;
    case ImageFormat::Bmp:  fileType = Gdiplus::ImageFormatBMP; break;
    case ImageFormat::Jpeg: fileType = Gdiplus::ImageFormatJPEG; break;
    }

    const HRESULT hr = image.Save(path, fileType);
    if (FAILED(hr))
    {
        errorMessage.Format(L"파일을 저장하지 못했습니다. 경로와 쓰기 권한을 확인해 주세요.\n\n%s\n(오류 0x%08X)",
                            static_cast<LPCWSTR>(path), static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

bool LoadImageFile(const CString& path, CCanvasSurface& surface, CString& errorMessage)
{
    // GDI+ Bitmap은 살아 있는 동안 파일을 열어 두므로 이 함수 안에서만 쓰고 바로 닫음
    Gdiplus::Bitmap bitmap(static_cast<LPCWSTR>(path));
    if (bitmap.GetLastStatus() != Gdiplus::Ok)
    {
        errorMessage = L"그림 파일을 열지 못했습니다. 파일이 있는지, PNG·BMP·JPEG 그림이 맞는지 확인해 주세요.";
        return false;
    }

    const UINT width = bitmap.GetWidth();
    const UINT height = bitmap.GetHeight();
    if (width == 0 || height == 0)
    {
        errorMessage = L"그림 크기가 0입니다.";
        return false;
    }
    if (width > kMaxImageSide || height > kMaxImageSide ||
        static_cast<unsigned long long>(width) * height > kMaxImagePixels)
    {
        errorMessage.Format(L"그림이 너무 큽니다 (%u × %upx). 한 변 %upx, 전체 %llu만 픽셀까지 열 수 있습니다.",
                            width, height, kMaxImageSide, kMaxImagePixels / 10000);
        return false;
    }
    if (!surface.Create(static_cast<int>(width), static_cast<int>(height), RGB(255, 255, 255)))
    {
        errorMessage = L"그림을 담을 메모리를 만들지 못했습니다. 메모리가 부족할 수 있습니다.";
        return false;
    }

    // 어떤 픽셀 형식이든(팔레트, 24비트, 알파 포함 등) 32비트 ARGB로 받아서 변환
    Gdiplus::Rect rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
    Gdiplus::BitmapData data{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
    {
        errorMessage = L"그림 픽셀을 읽지 못했습니다.";
        return false;
    }

    uint32_t* dst = surface.Pixels();
    for (UINT y = 0; y < height; ++y)
    {
        const auto* src = reinterpret_cast<const uint32_t*>(static_cast<const BYTE*>(data.Scan0) +
                                                            static_cast<INT_PTR>(y) * data.Stride);
        uint32_t* row = dst + static_cast<size_t>(y) * width;
        for (UINT x = 0; x < width; ++x)
        {
            const uint32_t argb = src[x];
            const uint32_t a = argb >> 24;
            if (a == 255)
            {
                row[x] = argb & 0x00FFFFFF; // 대부분의 불투명 픽셀은 그대로
                continue;
            }
            // 투명한 부분은 흰 바탕 위에 합성: c' = c × a + 255 × (1 - a)
            auto over = [a](uint32_t c) { return (c * a + 255 * (255 - a) + 127) / 255; };
            const uint32_t r = over((argb >> 16) & 0xFF);
            const uint32_t g = over((argb >> 8) & 0xFF);
            const uint32_t b = over(argb & 0xFF);
            row[x] = (r << 16) | (g << 8) | b;
        }
    }
    bitmap.UnlockBits(&data);
    return true;
}
