/**
 * @file ColorPalette.cpp
 * @brief 색 팔레트 컨트롤 구현
 */
#include "pch.h"
#include "ColorPalette.h"
#include "PaintTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    constexpr int kGap = 2; ///< 칸 사이 간격 (96 DPI 기준 px)

    const COLORREF kBorderColor = RGB(128, 128, 128);
    const COLORREF kInnerBorderColor = RGB(255, 255, 255);

    /** @brief 테두리가 있는 색 사각형을 그립니다. */
    void DrawSwatch(CDC& dc, const CRect& rc, COLORREF color)
    {
        dc.FillSolidRect(rc, kBorderColor);
        CRect inner(rc);
        inner.DeflateRect(1, 1);
        dc.FillSolidRect(inner, kInnerBorderColor);
        inner.DeflateRect(1, 1);
        dc.FillSolidRect(inner, color);
    }
}

BEGIN_MESSAGE_MAP(CColorPalette, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

CColorPalette::CColorPalette()
    : m_cells{
          // 열마다 짝: 검정/흰색, 빨강/노랑(따뜻한 색), 초록/파랑(차가운 색)
          RGB(0, 0, 0),       RGB(237, 28, 36),   RGB(34, 177, 76),   // 윗줄: 검정, 빨강, 초록
          RGB(255, 255, 255), RGB(255, 242, 0),   RGB(63, 72, 204),   // 아랫줄: 흰색, 노랑, 파랑
      }
{
    for (COLORREF& color : m_customColors)
    {
        color = RGB(255, 255, 255);
    }
}

BOOL CColorPalette::Create(const CRect& rect, CWnd* pParent, UINT nID)
{
    const CString className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_HAND), nullptr, nullptr);
    return CWnd::Create(className, nullptr, WS_CHILD | WS_VISIBLE, rect, pParent, nID);
}

int CColorPalette::CalcWidth(int height, int gap)
{
    const int cell = (height - gap) / 2;
    return height + 3 * gap + kColumns * cell + (kColumns - 1) * gap;
}

void CColorPalette::SetPrimary(COLORREF color)
{
    m_primary = color;
    if (GetSafeHwnd() != nullptr)
    {
        Invalidate(FALSE);
    }
}

void CColorPalette::SetSecondary(COLORREF color)
{
    m_secondary = color;
    if (GetSafeHwnd() != nullptr)
    {
        Invalidate(FALSE);
    }
}

bool CColorPalette::EditPrimary()
{
    CColorDialog dlg(m_primary, CC_FULLOPEN | CC_RGBINIT, this);
    dlg.m_cc.lpCustColors = m_customColors;
    if (dlg.DoModal() != IDOK)
    {
        return false;
    }
    SetPrimary(dlg.GetColor());
    NotifyParent();
    return true;
}

int CColorPalette::Scale(int value) const
{
    return ::MulDiv(value, static_cast<int>(::GetDpiForWindow(GetSafeHwnd())), USER_DEFAULT_SCREEN_DPI);
}

CColorPalette::Layout CColorPalette::CalcLayout() const
{
    CRect rcClient;
    GetClientRect(&rcClient);

    Layout layout;
    layout.gap = Scale(kGap);
    const int height = rcClient.Height();
    layout.cell = max(1, (height - layout.gap) / 2);

    layout.swatchArea.SetRect(0, 0, height, height);
    const int swatch = height * 5 / 8;
    layout.primary.SetRect(0, 0, swatch, swatch);
    layout.secondary.SetRect(height - swatch, height - swatch, height, height);

    layout.gridLeft = height + 3 * layout.gap;
    return layout;
}

CRect CColorPalette::CellRect(const Layout& layout, int index) const
{
    const int row = index / kColumns;
    const int col = index % kColumns;
    const int x = layout.gridLeft + col * (layout.cell + layout.gap);
    const int y = row * (layout.cell + layout.gap);
    return CRect(x, y, x + layout.cell, y + layout.cell);
}

int CColorPalette::HitTestCell(CPoint point) const
{
    const Layout layout = CalcLayout();
    for (int i = 0; i < static_cast<int>(m_cells.size()); ++i)
    {
        if (CellRect(layout, i).PtInRect(point))
        {
            return i;
        }
    }
    return -1;
}

void CColorPalette::NotifyParent()
{
    GetParent()->SendMessage(WM_PALETTE_COLORCHANGED);
}

BOOL CColorPalette::OnEraseBkgnd(CDC*)
{
    return TRUE;
}

void CColorPalette::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap buffer;
    buffer.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* oldBitmap = memDC.SelectObject(&buffer);

    memDC.FillSolidRect(rcClient, ::GetSysColor(COLOR_3DFACE));

    const Layout layout = CalcLayout();
    DrawSwatch(memDC, layout.secondary, m_secondary); // 색 2가 뒤
    DrawSwatch(memDC, layout.primary, m_primary);     // 색 1이 앞

    for (int i = 0; i < static_cast<int>(m_cells.size()); ++i)
    {
        DrawSwatch(memDC, CellRect(layout, i), m_cells[i]);
    }

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(oldBitmap);
}

void CColorPalette::OnLButtonDown(UINT, CPoint point)
{
    if (CalcLayout().swatchArea.PtInRect(point))
    {
        std::swap(m_primary, m_secondary);
        Invalidate(FALSE);
        NotifyParent();
        return;
    }
    const int index = HitTestCell(point);
    if (index >= 0)
    {
        SetPrimary(m_cells[index]);
        NotifyParent();
    }
}

void CColorPalette::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    const int index = HitTestCell(point);
    if (index < 0)
    {
        OnLButtonDown(nFlags, point); // 색 1/색 2 표시를 빠르게 두 번 눌러도 두 번 맞바꿈
        return;
    }

    CColorDialog dlg(m_cells[index], CC_FULLOPEN | CC_RGBINIT, this);
    dlg.m_cc.lpCustColors = m_customColors;
    if (dlg.DoModal() == IDOK)
    {
        m_cells[index] = dlg.GetColor();
        SetPrimary(m_cells[index]);
        NotifyParent();
    }
}

void CColorPalette::OnRButtonDown(UINT, CPoint point)
{
    const int index = HitTestCell(point);
    if (index >= 0)
    {
        SetSecondary(m_cells[index]);
        NotifyParent();
    }
}

void CColorPalette::OnRButtonUp(UINT, CPoint)
{
    // 기본 처리를 하지 않아 WM_CONTEXTMENU가 생기지 않게 함
}
