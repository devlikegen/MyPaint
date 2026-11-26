#pragma once

/**
 * @file ColorPalette.h
 * @brief 색 1/색 2 표시와 기본 색 상자
 */

#include <array>

/**
 * @brief 색 팔레트 컨트롤.
 *
 * 왼쪽에 색 1(앞)과 색 2(뒤)를 겹쳐 보여 주고, 오른쪽에 자주 쓰는 기본 색 6가지(2줄 × 3칸)를 보여 줍니다.
 * 그 밖의 색은 "색 편집" 버튼이나 색 칸 두 번 클릭으로 고릅니다.
 * - 색 칸을 왼쪽 클릭: 색 1, 오른쪽 클릭: 색 2로 정함
 * - 색 칸을 두 번 클릭: 그 칸의 색을 편집
 * - 색 1/색 2 표시를 클릭: 두 색을 맞바꿈
 * 색이 바뀌면 부모에 WM_PALETTE_COLORCHANGED를 보냅니다.
 */
class CColorPalette : public CWnd
{
public:
    /** @brief 기본 색 칸 수 (6가지) */
    static constexpr int kColumns = 3;
    static constexpr int kRows = 2;

    CColorPalette();

    /**
     * @brief 창을 만듭니다.
     * @param rect 부모 클라이언트 좌표의 위치
     * @param pParent 부모 창
     * @param nID 컨트롤 ID
     * @return 성공하면 TRUE
     */
    BOOL Create(const CRect& rect, CWnd* pParent, UINT nID);

    /**
     * @brief 주어진 높이에서 필요한 너비를 계산합니다.
     * @param height 컨트롤 높이(px)
     * @param gap 칸 사이 간격(px)
     */
    static int CalcWidth(int height, int gap);

    COLORREF GetPrimary() const { return m_primary; }
    COLORREF GetSecondary() const { return m_secondary; }

    /** @brief 색 1을 정합니다. 부모에 알리지 않습니다. */
    void SetPrimary(COLORREF color);
    /** @brief 색 2를 정합니다. 부모에 알리지 않습니다. */
    void SetSecondary(COLORREF color);

    /**
     * @brief 색 편집 대화 상자를 띄워 색 1을 바꿉니다.
     * @return 바꿨으면 true
     */
    bool EditPrimary();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

private:
    /** @brief 현재 크기에서의 배치 */
    struct Layout
    {
        CRect swatchArea;   ///< 색 1/색 2 표시 영역
        CRect primary;      ///< 색 1 사각형
        CRect secondary;    ///< 색 2 사각형
        int gridLeft = 0;   ///< 색 칸 시작 x
        int cell = 0;       ///< 색 칸 한 변
        int gap = 0;        ///< 칸 사이 간격
    };

    Layout CalcLayout() const;
    CRect CellRect(const Layout& layout, int index) const;
    /** @brief 점이 있는 색 칸 번호. 없으면 -1 */
    int HitTestCell(CPoint point) const;
    int Scale(int value) const;
    void NotifyParent();

    std::array<COLORREF, kColumns * kRows> m_cells; ///< 기본 색 (윗줄 → 아랫줄)
    COLORREF m_primary = RGB(0, 0, 0);
    COLORREF m_secondary = RGB(255, 255, 255);
    COLORREF m_customColors[16] = {};               ///< 색 대화 상자의 사용자 지정 색
};
