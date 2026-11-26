#pragma once

/**
 * @file pch.h
 * @brief 미리 컴파일된 헤더. 자주 바뀌지 않는 헤더를 여기에 추가합니다.
 */
#include "framework.h"

#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include <atlimage.h> // CImage: PNG/BMP/JPEG 저장
#include <shlwapi.h>  // PathFindFileName, PathFindExtension
#pragma comment(lib, "shlwapi.lib")

// GDI+: 앤티에일리어싱 선과 도형
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <array>
#include <memory>
#include <utility>
#include <vector>
