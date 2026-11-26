/**
 * @file CanvasSurface.cpp
 * @brief 그림 데이터 비트맵 구현
 */
#include "pch.h"
#include "CanvasSurface.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    /** @brief 픽셀 비교에서 무시할 알파 바이트를 뺀 마스크 */
    constexpr uint32_t kRgbMask = 0x00FFFFFF;

    /** @brief COLORREF(0x00BBGGRR) → DIB 픽셀(0x00RRGGBB) */
    uint32_t ToPixel(COLORREF color)
    {
        return (static_cast<uint32_t>(GetRValue(color)) << 16) |
               (static_cast<uint32_t>(GetGValue(color)) << 8) |
               static_cast<uint32_t>(GetBValue(color));
    }

    /** @brief DIB 픽셀(0x00RRGGBB) → COLORREF(0x00BBGGRR) */
    COLORREF ToColor(uint32_t pixel)
    {
        return RGB((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF);
    }
}

CCanvasSurface::~CCanvasSurface()
{
    Destroy();
}

bool CCanvasSurface::Create(int width, int height, COLORREF background)
{
    Destroy();
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 음수: 위에서 아래 순서
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = ::CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (hBitmap == nullptr)
    {
        return false;
    }
    if (!m_dc.CreateCompatibleDC(nullptr))
    {
        ::DeleteObject(hBitmap);
        return false;
    }

    m_hBitmap = hBitmap;
    m_hOldBitmap = ::SelectObject(m_dc.GetSafeHdc(), m_hBitmap);
    m_bits = static_cast<uint32_t*>(bits);
    m_width = width;
    m_height = height;

    m_dc.FillSolidRect(0, 0, width, height, background);
    return true;
}

void CCanvasSurface::Destroy()
{
    if (m_dc.GetSafeHdc() != nullptr)
    {
        if (m_hOldBitmap != nullptr)
        {
            ::SelectObject(m_dc.GetSafeHdc(), m_hOldBitmap);
        }
        m_dc.DeleteDC();
    }
    if (m_hBitmap != nullptr)
    {
        ::DeleteObject(m_hBitmap);
    }
    m_hBitmap = nullptr;
    m_hOldBitmap = nullptr;
    m_bits = nullptr;
    m_width = 0;
    m_height = 0;
}

bool CCanvasSurface::Contains(CPoint pt) const
{
    return pt.x >= 0 && pt.y >= 0 && pt.x < m_width && pt.y < m_height;
}

uint32_t* CCanvasSurface::Pixels()
{
    ::GdiFlush(); // 일괄 처리 중인 GDI 그리기가 픽셀 메모리에 반영되도록
    return m_bits;
}

COLORREF CCanvasSurface::GetPixel(CPoint pt)
{
    ASSERT(Contains(pt));
    if (!Contains(pt))
    {
        return RGB(0, 0, 0);
    }
    return ToColor(Pixels()[static_cast<size_t>(pt.y) * m_width + pt.x]);
}

CRect CCanvasSurface::FloodFill(CPoint seed, COLORREF color, int tolerance)
{
    CRect dirty;
    dirty.SetRectEmpty();
    if (!Contains(seed))
    {
        return dirty;
    }

    uint32_t* pixels = Pixels();
    const uint32_t replacement = ToPixel(color);
    const uint32_t target = pixels[static_cast<size_t>(seed.y) * m_width + seed.x] & kRgbMask;
    if (target == replacement && tolerance <= 0)
    {
        return dirty; // 이미 같은 색
    }

    auto index = [&](int x, int y) { return static_cast<size_t>(y) * m_width + x; };
    auto similar = [&](uint32_t pixel) {
        const int dr = std::abs(static_cast<int>((pixel >> 16) & 0xFF) - static_cast<int>((target >> 16) & 0xFF));
        const int dg = std::abs(static_cast<int>((pixel >> 8) & 0xFF) - static_cast<int>((target >> 8) & 0xFF));
        const int db = std::abs(static_cast<int>(pixel & 0xFF) - static_cast<int>(target & 0xFF));
        return max(dr, max(dg, db)) <= tolerance;
    };
    // 채운 색이 허용 범위 안일 수도 있으므로, 픽셀 값이 아니라 방문 표시로 다시 채우지 않게 함
    std::vector<uint8_t> visited(static_cast<size_t>(m_width) * m_height, 0);
    auto at = [&](int x, int y) -> uint32_t& { return pixels[index(x, y)]; };
    auto isTarget = [&](int x, int y) { return visited[index(x, y)] == 0 && similar(at(x, y)); };

    // 스캔라인 채우기: 가로 구간을 한 번에 채우고, 위/아래 줄에서 새 구간의 시작점만 스택에 넣습니다.
    std::vector<POINT> stack;
    stack.push_back(seed);
    int minX = seed.x, maxX = seed.x, minY = seed.y, maxY = seed.y;

    while (!stack.empty())
    {
        const POINT p = stack.back();
        stack.pop_back();
        if (!isTarget(p.x, p.y))
        {
            continue; // 다른 구간을 채우면서 이미 채워짐
        }

        int left = p.x;
        while (left > 0 && isTarget(left - 1, p.y))
        {
            --left;
        }
        int right = p.x;
        while (right < m_width - 1 && isTarget(right + 1, p.y))
        {
            ++right;
        }

        for (int x = left; x <= right; ++x)
        {
            at(x, p.y) = replacement;
            visited[index(x, p.y)] = 1;
        }
        minX = min(minX, left);
        maxX = max(maxX, right);
        minY = min(minY, static_cast<int>(p.y));
        maxY = max(maxY, static_cast<int>(p.y));

        for (int ny : { p.y - 1, p.y + 1 })
        {
            if (ny < 0 || ny >= m_height)
            {
                continue;
            }
            bool inRun = false;
            for (int x = left; x <= right; ++x)
            {
                const bool match = isTarget(x, ny);
                if (match && !inRun)
                {
                    stack.push_back(POINT{ x, ny });
                }
                inRun = match;
            }
        }
    }

    BlendFillEdge(visited, target, replacement, CRect(minX, minY, maxX + 1, maxY + 1));

    dirty.SetRect(minX - 1, minY - 1, maxX + 2, maxY + 2); // 경계 다듬기로 1픽셀 바깥까지 바뀔 수 있음
    dirty.IntersectRect(dirty, GetRect());
    return dirty;
}

void CCanvasSurface::BlendFillEdge(const std::vector<uint8_t>& filled, uint32_t target, uint32_t replacement,
                                   const CRect& filledBounds)
{
    // 채운 영역 바로 바깥 픽셀은 앤티에일리어싱으로 "원래 색(target)"과 "경계 색"이 섞여 있을 수 있습니다.
    // 주변 3×3에서 target과 가장 먼 색을 경계 색 O로 보고, 픽셀 p를 O와 target의 섞임으로 풀어
    // target이 차지하던 비율 k만큼을 채울 색으로 바꿉니다: p' = p + k × (채울 색 - target).
    // 섞임이 아닌 픽셀(자기 자신이 가장 먼 색)은 k = 0이라 바뀌지 않습니다.
    struct Rgb { int r, g, b; };
    auto toRgb = [](uint32_t px) { return Rgb{ static_cast<int>((px >> 16) & 0xFF), static_cast<int>((px >> 8) & 0xFF), static_cast<int>(px & 0xFF) }; };
    auto distSq = [](const Rgb& a, const Rgb& b) { return (a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b); };

    const Rgb t = toRgb(target);
    const Rgb f = toRgb(replacement);
    auto isFilled = [&](int x, int y) { return filled[static_cast<size_t>(y) * m_width + x] != 0; };

    struct Write { size_t index; uint32_t pixel; };
    std::vector<Write> writes; // 다른 경계 픽셀의 계산에 영향을 주지 않도록 모아서 한 번에 씀

    const int x0 = max(0, filledBounds.left - 1), x1 = min(m_width - 1, filledBounds.right);
    const int y0 = max(0, filledBounds.top - 1), y1 = min(m_height - 1, filledBounds.bottom);
    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            if (isFilled(x, y))
            {
                continue;
            }
            const bool touchesFill = (x > 0 && isFilled(x - 1, y)) || (x < m_width - 1 && isFilled(x + 1, y)) ||
                                     (y > 0 && isFilled(x, y - 1)) || (y < m_height - 1 && isFilled(x, y + 1));
            if (!touchesFill)
            {
                continue;
            }

            const size_t i = static_cast<size_t>(y) * m_width + x;
            const Rgb p = toRgb(m_bits[i]);

            Rgb o = p;
            int farthest = distSq(p, t);
            for (int ny = max(0, y - 1); ny <= min(m_height - 1, y + 1); ++ny)
            {
                for (int nx = max(0, x - 1); nx <= min(m_width - 1, x + 1); ++nx)
                {
                    if (isFilled(nx, ny))
                    {
                        continue;
                    }
                    const Rgb n = toRgb(m_bits[static_cast<size_t>(ny) * m_width + nx]);
                    const int d = distSq(n, t);
                    if (d > farthest)
                    {
                        farthest = d;
                        o = n;
                    }
                }
            }
            if (farthest == 0)
            {
                continue;
            }

            // p를 O→target 선분에 투영한 위치가 target 비율 k
            const double k = static_cast<double>((p.r - o.r) * (t.r - o.r) + (p.g - o.g) * (t.g - o.g) + (p.b - o.b) * (t.b - o.b)) /
                             static_cast<double>(distSq(t, o));
            if (k <= 0.0)
            {
                continue;
            }
            const double kk = min(k, 1.0);
            auto channel = [kk](int pc, int fc, int tc) { return max(0, min(255, static_cast<int>(pc + kk * (fc - tc) + 0.5))); };
            const uint32_t blended = (static_cast<uint32_t>(channel(p.r, f.r, t.r)) << 16) |
                                     (static_cast<uint32_t>(channel(p.g, f.g, t.g)) << 8) |
                                     static_cast<uint32_t>(channel(p.b, f.b, t.b));
            writes.push_back(Write{ i, blended });
        }
    }

    for (const Write& w : writes)
    {
        m_bits[w.index] = w.pixel;
    }
}

bool CCanvasSurface::ContentEquals(CCanvasSurface& other)
{
    if (&other == this)
    {
        return true;
    }
    if (!IsValid() || !other.IsValid() || GetSize() != other.GetSize())
    {
        return IsValid() == other.IsValid() && GetSize() == other.GetSize();
    }
    // 알파 바이트는 GDI/GDI+가 제각각 쓰므로 RGB만 비교
    const uint32_t* a = Pixels();
    const uint32_t* b = other.Pixels();
    const size_t count = static_cast<size_t>(m_width) * m_height;
    for (size_t i = 0; i < count; ++i)
    {
        if (((a[i] ^ b[i]) & kRgbMask) != 0)
        {
            return false;
        }
    }
    return true;
}

bool CCanvasSurface::CopyFrom(CCanvasSurface& other)
{
    if (&other == this || !other.IsValid())
    {
        return false;
    }
    if (GetSize() != other.GetSize())
    {
        if (!Create(other.m_width, other.m_height, RGB(255, 255, 255)))
        {
            return false;
        }
    }
    std::memcpy(Pixels(), other.Pixels(), static_cast<size_t>(m_width) * m_height * sizeof(uint32_t));
    return true;
}
