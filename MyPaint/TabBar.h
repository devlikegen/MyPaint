#pragma once

/**
 * @file TabBar.h
 * @brief 크롬 스타일 탭 표시줄
 */

#include "PaintTypes.h"

#include <vector>

/**
 * @brief 크롬 스타일 탭 표시줄 컨트롤.
 *
 * 탭 이름과 변경 표시만 보여 주는 화면 부품이고, 탭이 가리키는 문서는 부모가 관리합니다.
 * - 탭 클릭: 선택, 탭의 × 또는 가운데 버튼 클릭: 닫기
 * - + 단추 또는 빈 곳 두 번 클릭: 새 탭
 * - 저장하지 않은 변경이 있는 탭은 × 자리에 ●를 표시하고, 마우스를 올리면 ×로 바뀜
 * - 고스트 원본 탭은 이름 앞에 작은 유령 모양을 표시 (SetGhostMark)
 * - 탭이 많아지면 크롬처럼 탭 너비가 줄어듦
 * 사용자 조작은 부모에 WM_TABBAR_ACTION으로 알리고, 실제 선택/닫기는 부모가 결정합니다.
 */
class CTabBar : public CWnd
{
public:
    /** @brief 고스트 원본 탭 이름 앞에 붙이는 표시 */
    enum class GhostMark
    {
        None,          ///< 표시 없음
        Source,        ///< 고스트가 이 탭을 비추는 중 (고정 없이 고른 바로 왼쪽 탭): 보라색 테두리 유령
        PinnedSource,  ///< 고스트가 켜져 있고 이 탭이 고정된 원본: 보라색 꽉 찬 유령
        PinnedIdle,    ///< 원본으로 고정했지만 고스트는 꺼져 있음: 회색 꽉 찬 유령
    };

    /**
     * @brief 창을 만듭니다.
     * @param rect 부모 클라이언트 좌표의 위치
     * @param pParent 부모 창
     * @param nID 컨트롤 ID
     * @return 성공하면 TRUE
     */
    BOOL Create(const CRect& rect, CWnd* pParent, UINT nID);

    /** @brief 현재 DPI에서 알맞은 높이(px) */
    int GetPreferredHeight() const;

    /**
     * @brief 탭을 추가합니다.
     * @param index 넣을 위치 (탭 개수이면 맨 끝)
     * @param title 탭 이름
     */
    void InsertTab(int index, const CString& title);

    /** @brief 탭을 지웁니다. 선택은 부모가 SetActiveTab으로 다시 정합니다. */
    void RemoveTab(int index);

    /**
     * @brief 탭 이름과 변경 표시를 바꿉니다.
     * @param index 탭 번호
     * @param title 탭 이름
     * @param modified 저장하지 않은 변경이 있으면 true
     */
    void SetTab(int index, const CString& title, bool modified);

    /** @brief 선택된 탭을 바꿉니다. */
    void SetActiveTab(int index);

    /**
     * @brief 고스트 원본 표시를 정합니다. 표시는 한 탭에만 붙고, 다른 탭의 표시는 지워집니다.
     * @param index 표시할 탭 번호 (-1이면 모두 지움)
     * @param mark 표시 종류
     */
    void SetGhostMark(int index, GhostMark mark);

    int GetActiveTab() const { return m_active; }
    int GetTabCount() const { return static_cast<int>(m_tabs.size()); }

    /** @brief 그림 영역과 이어지도록 선택된 탭을 칠할 색 (캔버스 작업 영역 색) */
    void SetActiveColor(COLORREF color) { m_activeColor = color; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetFont(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    /** @brief 탭 하나의 표시 정보 */
    struct Tab
    {
        CString title;
        bool modified = false;
    };

    /** @brief 마우스가 가리키는 부분 */
    struct HitResult
    {
        int tab = -1;          ///< 탭 번호 (-1: 탭 아님)
        bool close = false;    ///< 탭의 × 단추 위
        bool newTab = false;   ///< + 단추 위
    };

    int Scale(int value) const;
    /** @brief 현재 크기에서 탭 하나의 너비 */
    int TabWidth() const;
    /** @brief 탭이 차지하는 영역 (둥근 아래 모서리 제외) */
    CRect TabRect(int index) const;
    /** @brief 탭 안의 × 단추 영역 */
    CRect CloseRect(int index) const;
    /** @brief + 단추 영역 */
    CRect NewTabRect() const;
    HitResult HitTest(CPoint point) const;

    void Notify(TabBarAction action, int index);
    void SetHover(const HitResult& hit);

    void DrawTab(Gdiplus::Graphics& g, CDC& dc, int index) const;

    std::vector<Tab> m_tabs;
    int m_active = -1;
    HitResult m_hover;              ///< 마우스가 올라가 있는 부분
    int m_pressedClose = -1;        ///< × 단추를 누른 탭 (떼었을 때 같은 단추 위면 닫음)
    bool m_trackingMouse = false;
    HFONT m_hFont = nullptr;        ///< 부모가 WM_SETFONT로 준 글꼴 (소유하지 않음)
    COLORREF m_activeColor = RGB(255, 255, 255);
    int m_ghostTab = -1;            ///< 고스트 원본 표시를 붙인 탭 (-1: 없음)
    GhostMark m_ghostMark = GhostMark::None;
};
