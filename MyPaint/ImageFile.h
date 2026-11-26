#pragma once

/**
 * @file ImageFile.h
 * @brief 그림을 이미지 파일로 저장하고 불러오기
 */

class CCanvasSurface;

/** @brief 저장할 수 있는 이미지 형식 */
enum class ImageFormat
{
    Png,
    Bmp,
    Jpeg,
};

/**
 * @brief 파일 확장자로 이미지 형식을 알아냅니다 (.png, .bmp, .jpg, .jpeg, 대소문자 무시).
 * @param path 파일 경로
 * @param format [out] 형식
 * @return 지원하는 확장자이면 true
 */
bool ImageFormatFromPath(const CString& path, ImageFormat& format);

/**
 * @brief 그림을 파일로 저장합니다. 형식은 확장자로 정합니다.
 *
 * 그림의 픽셀 메모리는 알파 바이트가 0이므로, 투명한 PNG가 되지 않도록 24비트로 바꿔 저장합니다.
 * @param surface 저장할 그림
 * @param path 저장 경로
 * @param errorMessage [out] 실패했을 때 사용자에게 보여 줄 이유
 * @return 성공하면 true
 */
bool SaveImageFile(CCanvasSurface& surface, const CString& path, CString& errorMessage);

/**
 * @brief 이미지 파일을 읽어 그림을 만듭니다 (PNG, BMP, JPEG. GDI+가 읽을 수 있는 GIF, TIFF도 가능).
 *
 * 투명한 부분은 흰 바탕 위에 합성합니다 (그림판과 같음). 여러 장으로 된 그림은 첫 장만 읽습니다.
 * @param path 파일 경로
 * @param surface [out] 읽은 그림. 실패하면 내용을 보장하지 않으므로 새로 만든 그림을 넘기세요.
 * @param errorMessage [out] 실패했을 때 사용자에게 보여 줄 이유
 * @return 성공하면 true
 */
bool LoadImageFile(const CString& path, CCanvasSurface& surface, CString& errorMessage);
