#pragma once

/**
 * @file CanvasSurface.h
 * @brief 그림 데이터를 담는 32비트 DIB 비트맵
 */

#include <cstdint>
#include <vector>

/**
 * @brief 그림 한 장의 픽셀 데이터.
 *
 * DIB 섹션을 메모리 DC에 선택해 두므로 GDI/GDI+로 그릴 수 있고,
 * 동시에 픽셀 메모리에 직접 접근할 수 있습니다(채우기, 스포이트).
 * 픽셀은 위에서 아래 순서, 0x00RRGGBB 형식입니다.
 */
class CCanvasSurface
{
public:
    CCanvasSurface() = default;
    ~CCanvasSurface();

    CCanvasSurface(const CCanvasSurface&) = delete;
    CCanvasSurface& operator=(const CCanvasSurface&) = delete;

    /**
     * @brief 지정한 크기의 비트맵을 만들고 배경색으로 채웁니다. 기존 내용은 버립니다.
     * @param width 너비(px)
     * @param height 높이(px)
     * @param background 배경색
     * @return 성공하면 true
     */
    bool Create(int width, int height, COLORREF background);

    /** @brief 비트맵을 해제합니다. */
    void Destroy();

    /** @brief 비트맵이 만들어져 있으면 true */
    bool IsValid() const { return m_hBitmap != nullptr; }

    /** @brief 비트맵이 선택된 메모리 DC. 여기에 그리면 그림에 반영됩니다. */
    CDC& GetDC() { return m_dc; }

    /** @brief 그림 크기(px) */
    CSize GetSize() const { return CSize(m_width, m_height); }

    /** @brief 그림 전체 영역 (0, 0, 너비, 높이) */
    CRect GetRect() const { return CRect(0, 0, m_width, m_height); }

    /** @brief 점이 그림 안에 있으면 true */
    bool Contains(CPoint pt) const;

    /**
     * @brief 픽셀 색을 읽습니다.
     * @param pt 그림 좌표. 그림 안이어야 합니다.
     */
    COLORREF GetPixel(CPoint pt);

    /**
     * @brief 시작점과 비슷한 색으로 이어진 영역을 채웁니다 (4방향).
     * @param seed 시작점
     * @param color 채울 색
     * @param tolerance 시작점 색과 R, G, B 각각의 차이가 이 값 이하이면 같은 색으로 봄 (0~255).
     *        앤티에일리어싱된 경계의 옅은 픽셀까지 채우는 데 씁니다.
     * @return 바뀐 영역. 바뀐 픽셀이 없으면 빈 사각형
     */
    CRect FloodFill(CPoint seed, COLORREF color, int tolerance);

    /**
     * @brief 다른 그림의 내용을 그대로 복사합니다. 크기가 다르면 크기도 맞춥니다.
     * @param other 원본
     * @return 성공하면 true
     */
    bool CopyFrom(CCanvasSurface& other);

    /**
     * @brief 다른 그림과 크기와 픽셀이 모두 같은지 비교합니다.
     * @param other 비교할 그림
     * @return 같으면 true
     */
    bool ContentEquals(CCanvasSurface& other);

    /**
     * @brief 픽셀 메모리. GDI가 아직 쓰지 않은 그리기를 먼저 마칩니다.
     *
     * 위에서 아래 순서로 한 줄에 GetSize().cx개, 0x00RRGGBB 형식입니다 (파일 불러오기 등에서 직접 씀).
     * 그림이 없으면 nullptr입니다.
     */
    uint32_t* Pixels();

private:
    /**
     * @brief 채운 영역 바로 바깥의 앤티에일리어싱 픽셀에서 원래 색 비율만큼을 채울 색으로 바꿉니다.
     *        (채운 뒤 경계에 옅은 테두리가 남지 않게 함)
     * @param filled 픽셀별로 채웠으면 1
     * @param target 채우기 전 시작점 색 (0x00RRGGBB)
     * @param replacement 채울 색 (0x00RRGGBB)
     * @param filledBounds 채운 픽셀을 모두 포함하는 사각형
     */
    void BlendFillEdge(const std::vector<uint8_t>& filled, uint32_t target, uint32_t replacement, const CRect& filledBounds);

    CDC m_dc;                        ///< 비트맵이 선택된 메모리 DC
    HBITMAP m_hBitmap = nullptr;     ///< DIB 섹션
    HGDIOBJ m_hOldBitmap = nullptr;  ///< DC에 원래 선택돼 있던 비트맵
    uint32_t* m_bits = nullptr;      ///< 픽셀 메모리 (DIB 섹션 소유)
    int m_width = 0;
    int m_height = 0;
};
