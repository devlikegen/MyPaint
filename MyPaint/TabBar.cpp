/**
 * @file TabBar.cpp
 * @brief 크롬 스타일 탭 표시줄 구현
 */
#include "pch.h"
#include "TabBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace Gdiplus;

namespace
{
    // 배치 값은 모두 96 DPI 기준 px이며 Scale()로 바꿔 씁니다.
    constexpr int kBarHeight = 36;      ///< 표시줄 높이
    constexpr int kTabTop = 6;          ///< 탭 위 여백
    constexpr int kSidePadding = 8;     ///< 왼쪽/오른쪽 여백
    constexpr int kCornerRadius = 8;    ///< 탭 모서리 반지름 (아래쪽 바깥 곡선도 같은 크기)
    constexpr int kTabMaxWidth = 220;   ///< 탭 최대 너비
    constexpr int kTabMinWidth = 56;    ///< 탭 최소 너비
    constexpr int kTextPadding = 12;    ///< 탭 왼쪽 글자 여백
    constexpr int kCloseSize = 18;      ///< × 단추 크기
    constexpr int kCloseMargin = 8;     ///< × 단추 오른쪽 여백
    constexpr int kCloseMinTabWidth = 90; ///< 이보다 좁은 탭은 선택된 탭에만 × 표시 (크롬과 같음)
    constexpr int kNewTabSize = 28;     ///< + 단추 크기
    constexpr int kNewTabGap = 6;       ///< 마지막 탭과 + 단추 사이
    constexpr int kGhostIconWidth = 12; ///< 고스트 원본 표시(유령 모양) 너비
    constexpr int kGhostIconHeight = 14; ///< 고스트 원본 표시 높이
    constexpr int kGhostIconGap = 5;    ///< 고스트 원본 표시와 탭 이름 사이

    const COLORREF kInactiveTextColor = RGB(70, 74, 80);
    const COLORREF kActiveTextColor = RGB(20, 20, 20);
    const COLORREF kSeparatorColor = RGB(165, 170, 180);
    const Color kGlyphColor(255, 80, 84, 90);       ///< ×, ●, + 색
    const Color kButtonHoverColor(40, 0, 0, 0);     ///< 단추 위에 마우스가 있을 때 덧칠할 색
    const Color kGhostOnColor(255, 112, 80, 200);   ///< 고스트가 켜져 있을 때 원본 표시 색 (보라)
    const Color kGhostIdleColor(255, 128, 132, 140); ///< 고스트가 꺼져 있을 때 고정 원본 표시 색 (회색)

    Color ToColor(COLORREF c, BYTE alpha = 255)
    {
        return Color(alpha, GetRValue(c), GetGValue(c), GetBValue(c));
    }

    COLORREF Mix(COLORREF a, COLORREF b, int percentB)
    {
        auto mix = [percentB](int x, int y) { return x + (y - x) * percentB / 100; };
        return RGB(mix(GetRValue(a), GetRValue(b)), mix(GetGValue(a), GetGValue(b)), mix(GetBValue(a), GetBValue(b)));
    }

    /** @brief 위쪽은 둥글고 아래쪽은 바깥으로 휘어 바닥과 이어지는 크롬 탭 모양 */
    void AddTabShape(GraphicsPath& path, const CRect& rc, int radius)
    {
        const REAL r = static_cast<REAL>(radius);
        const REAL l = static_cast<REAL>(rc.left), t = static_cast<REAL>(rc.top);
        const REAL rt = static_cast<REAL>(rc.right), b = static_cast<REAL>(rc.bottom);
        path.StartFigure();
        path.AddArc(l - 2 * r, b - 2 * r, 2 * r, 2 * r, 90.0f, -90.0f);   // 왼쪽 아래 바깥 곡선
        path.AddArc(l, t, 2 * r, 2 * r, 180.0f, 90.0f);                   // 왼쪽 위
        path.AddArc(rt - 2 * r, t, 2 * r, 2 * r, 270.0f, 90.0f);          // 오른쪽 위
        path.AddArc(rt, b - 2 * r, 2 * r, 2 * r, 180.0f, -90.0f);         // 오른쪽 아래 바깥 곡선
        path.CloseFigure();
    }

    void DrawCross(Graphics& g, const CRect& rc, REAL penWidth)
    {
        Pen pen(kGlyphColor, penWidth);
        const REAL inset = rc.Width() * 0.3f;
        const REAL l = rc.left + inset, t = rc.top + inset;
        const REAL r = rc.right - inset, b = rc.bottom - inset;
        g.DrawLine(&pen, PointF(l, t), PointF(r, b));
        g.DrawLine(&pen, PointF(r, t), PointF(l, b));
    }

    /**
     * @brief 작은 유령 모양 (고스트 원본 표시): 위는 둥글고 아래는 물결, 눈 두 개.
     * @param filled true이면 꽉 채우고 눈을 흰색으로, false이면 테두리만 그림
     */
    void DrawGhostIcon(Graphics& g, const RectF& box, const Color& color, bool filled, REAL penWidth)
    {
        const REAL l = box.X, t = box.Y, w = box.Width, h = box.Height;
        const REAL b = t + h;
        const REAL wave = h * 0.16f; // 아래 물결 깊이

        GraphicsPath path;
        path.AddArc(l, t, w, w, 180.0f, 180.0f);          // 머리
        path.AddLine(l + w, t + w / 2, l + w, b);         // 오른쪽 몸
        const PointF hem[] = {                            // 아래 물결 (오른쪽 → 왼쪽)
            PointF(l + w * 5 / 6, b - wave), PointF(l + w * 4 / 6, b),
            PointF(l + w * 3 / 6, b - wave), PointF(l + w * 2 / 6, b),
            PointF(l + w * 1 / 6, b - wave), PointF(l, b),
        };
        path.AddLines(hem, static_cast<INT>(_countof(hem)));
        path.CloseFigure();                               // 왼쪽 몸

        const REAL eyeW = w * 0.2f, eyeH = h * 0.22f, eyeY = t + w * 0.36f;
        SolidBrush body(color);
        SolidBrush eyes(filled ? Color(255, 255, 255, 255) : color);
        if (filled)
        {
            g.FillPath(&body, &path);
        }
        else
        {
            Pen pen(color, penWidth);
            g.DrawPath(&pen, &path);
        }
        g.FillEllipse(&eyes, l + w * 0.32f - eyeW / 2, eyeY, eyeW, eyeH);
        g.FillEllipse(&eyes, l + w * 0.68f - eyeW / 2, eyeY, eyeW, eyeH);
    }
}

BEGIN_MESSAGE_MAP(CTabBar, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_MBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_CAPTURECHANGED()
    ON_MESSAGE(WM_SETFONT, &CTabBar::OnSetFont)
    ON_MESSAGE(WM_GETFONT, &CTabBar::OnGetFont)
END_MESSAGE_MAP()

BOOL CTabBar::Create(const CRect& rect, CWnd* pParent, UINT nID)
{
    const CString className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return CWnd::Create(className, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rect, pParent, nID);
}

int CTabBar::Scale(int value) const
{
    const UINT dpi = (GetSafeHwnd() != nullptr) ? ::GetDpiForWindow(GetSafeHwnd()) : USER_DEFAULT_SCREEN_DPI;
    return ::MulDiv(value, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
}

int CTabBar::GetPreferredHeight() const
{
    return Scale(kBarHeight);
}

// ---------------------------------------------------------------------------
// 탭 목록
// ---------------------------------------------------------------------------

void CTabBar::InsertTab(int index, const CString& title)
{
    index = max(0, min(index, GetTabCount()));
    Tab tab;
    tab.title = title;
    m_tabs.insert(m_tabs.begin() + index, tab);
    if (m_active >= index)
    {
        ++m_active;
    }
    if (m_ghostTab >= index)
    {
        ++m_ghostTab;
    }
    m_hover = HitResult{};
    Invalidate(FALSE);
}

void CTabBar::RemoveTab(int index)
{
    if (index < 0 || index >= GetTabCount())
    {
        return;
    }
    m_tabs.erase(m_tabs.begin() + index);
    if (m_active == index)
    {
        m_active = -1;
    }
    else if (m_active > index)
    {
        --m_active;
    }
    if (m_ghostTab == index)
    {
        m_ghostTab = -1;
        m_ghostMark = GhostMark::None;
    }
    else if (m_ghostTab > index)
    {
        --m_ghostTab;
    }
    m_hover = HitResult{};
    m_pressedClose = -1;
    Invalidate(FALSE);
}

void CTabBar::SetTab(int index, const CString& title, bool modified)
{
    if (index < 0 || index >= GetTabCount())
    {
        return;
    }
    m_tabs[index].title = title;
    m_tabs[index].modified = modified;
    Invalidate(FALSE);
}

void CTabBar::SetActiveTab(int index)
{
    m_active = (index >= 0 && index < GetTabCount()) ? index : -1;
    Invalidate(FALSE);
}

void CTabBar::SetGhostMark(int index, GhostMark mark)
{
    if (index < 0 || index >= GetTabCount() || mark == GhostMark::None)
    {
        index = -1;
        mark = GhostMark::None;
    }
    if (index != m_ghostTab || mark != m_ghostMark)
    {
        m_ghostTab = index;
        m_ghostMark = mark;
        Invalidate(FALSE);
    }
}

// ---------------------------------------------------------------------------
// 배치
// ---------------------------------------------------------------------------

int CTabBar::TabWidth() const
{
    if (m_tabs.empty())
    {
        return Scale(kTabMaxWidth);
    }
    CRect rcClient;
    GetClientRect(&rcClient);
    const int left = Scale(kSidePadding) + Scale(kCornerRadius);
    const int reserved = Scale(kNewTabGap) + Scale(kNewTabSize) + Scale(kSidePadding) + Scale(kCornerRadius);
    const int available = rcClient.Width() - left - reserved;
    const int width = available / GetTabCount();
    return max(Scale(kTabMinWidth), min(width, Scale(kTabMaxWidth)));
}

CRect CTabBar::TabRect(int index) const
{
    CRect rcClient;
    GetClientRect(&rcClient);
    const int width = TabWidth();
    const int left = Scale(kSidePadding) + Scale(kCornerRadius) + index * width;
    return CRect(left, Scale(kTabTop), left + width, rcClient.bottom);
}

CRect CTabBar::CloseRect(int index) const
{
    const CRect tab = TabRect(index);
    const int size = Scale(kCloseSize);
    const int right = tab.right - Scale(kCloseMargin);
    const int top = tab.top + (tab.Height() - size) / 2;
    return CRect(right - size, top, right, top + size);
}

CRect CTabBar::NewTabRect() const
{
    const int size = Scale(kNewTabSize);
    const int left = TabRect(GetTabCount()).left + Scale(kNewTabGap);
    const CRect tabArea = TabRect(0);
    const int top = tabArea.top + (tabArea.Height() - size) / 2;
    return CRect(left, top, left + size, top + size);
}

CTabBar::HitResult CTabBar::HitTest(CPoint point) const
{
    HitResult hit;
    if (NewTabRect().PtInRect(point))
    {
        hit.newTab = true;
        return hit;
    }
    for (int i = 0; i < GetTabCount(); ++i)
    {
        if (TabRect(i).PtInRect(point))
        {
            hit.tab = i;
            const bool closeVisible = (i == m_active) || TabWidth() >= Scale(kCloseMinTabWidth);
            hit.close = closeVisible && CloseRect(i).PtInRect(point);
            return hit;
        }
    }
    return hit;
}

// ---------------------------------------------------------------------------
// 그리기
// ---------------------------------------------------------------------------

BOOL CTabBar::OnEraseBkgnd(CDC*)
{
    return TRUE;
}

void CTabBar::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    Invalidate(FALSE); // 탭 너비가 창 너비에 따라 바뀜
}

void CTabBar::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap buffer;
    buffer.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* oldBitmap = memDC.SelectObject(&buffer);

    const COLORREF background = ::GetSysColor(COLOR_3DFACE);
    memDC.FillSolidRect(rcClient, background);

    {
        Graphics g(memDC.GetSafeHdc());
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        // 마우스가 올라간 선택 안 된 탭: 옅게 칠함
        if (m_hover.tab >= 0 && m_hover.tab != m_active)
        {
            GraphicsPath path;
            AddTabShape(path, TabRect(m_hover.tab), Scale(kCornerRadius));
            SolidBrush brush(ToColor(Mix(background, m_activeColor, 45)));
            g.FillPath(&brush, &path);
        }

        // 탭 사이 구분선: 선택되거나 마우스가 올라간 탭 옆에는 그리지 않음 (크롬과 같음)
        Pen separator(ToColor(kSeparatorColor), 1.0f);
        for (int i = 1; i < GetTabCount(); ++i)
        {
            const bool hidden = (i == m_active || i - 1 == m_active || i == m_hover.tab || i - 1 == m_hover.tab);
            if (!hidden)
            {
                const CRect rc = TabRect(i);
                const REAL x = rc.left - 0.5f;
                g.DrawLine(&separator, PointF(x, static_cast<REAL>(rc.top + Scale(8))),
                           PointF(x, static_cast<REAL>(rc.bottom - Scale(6))));
            }
        }

        // 선택된 탭: 아래 캔버스 영역과 같은 색으로 이어지게 칠함
        if (m_active >= 0)
        {
            GraphicsPath path;
            AddTabShape(path, TabRect(m_active), Scale(kCornerRadius));
            SolidBrush brush(ToColor(m_activeColor));
            g.FillPath(&brush, &path);
        }

        g.Flush(FlushIntentionSync); // 아래 GDI 글자 그리기 전에 반영

        for (int i = 0; i < GetTabCount(); ++i)
        {
            DrawTab(g, memDC, i);
        }

        // + 단추
        const CRect rcNew = NewTabRect();
        if (m_hover.newTab)
        {
            SolidBrush hoverBrush(kButtonHoverColor);
            g.FillEllipse(&hoverBrush, static_cast<REAL>(rcNew.left), static_cast<REAL>(rcNew.top),
                          static_cast<REAL>(rcNew.Width()), static_cast<REAL>(rcNew.Height()));
        }
        Pen plusPen(kGlyphColor, static_cast<REAL>(Scale(2)) * 0.8f);
        const REAL cx = rcNew.left + rcNew.Width() / 2.0f;
        const REAL cy = rcNew.top + rcNew.Height() / 2.0f;
        const REAL arm = rcNew.Width() * 0.22f;
        g.DrawLine(&plusPen, PointF(cx - arm, cy), PointF(cx + arm, cy));
        g.DrawLine(&plusPen, PointF(cx, cy - arm), PointF(cx, cy + arm));
    }

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(oldBitmap);
}

void CTabBar::DrawTab(Graphics& g, CDC& dc, int index) const
{
    const Tab& tab = m_tabs[index];
    const CRect rcTab = TabRect(index);
    const CRect rcClose = CloseRect(index);
    const bool active = (index == m_active);
    const bool hovered = (index == m_hover.tab);
    const bool closeAllowed = active || TabWidth() >= Scale(kCloseMinTabWidth);

    // × 자리에 표시할 것: 마우스가 올라가 있으면 ×, 아니면 저장 안 된 변경 표시 ●, 선택된 탭은 ×
    enum class Glyph { None, Close, Modified } glyph = Glyph::None;
    if (hovered && closeAllowed)
    {
        glyph = Glyph::Close;
    }
    else if (tab.modified)
    {
        glyph = Glyph::Modified;
    }
    else if (active)
    {
        glyph = Glyph::Close;
    }

    // 제목
    CRect rcText(rcTab.left + Scale(kTextPadding), rcTab.top, rcClose.left - Scale(4), rcTab.bottom);
    if (glyph == Glyph::None && !closeAllowed)
    {
        rcText.right = rcTab.right - Scale(kCloseMargin);
    }

    // 고스트 원본 표시: 이름 앞의 작은 유령 (켜져 있으면 보라, 고정이면 꽉 참)
    if (index == m_ghostTab && m_ghostMark != GhostMark::None)
    {
        const int iconW = Scale(kGhostIconWidth);
        const int iconH = Scale(kGhostIconHeight);
        const RectF box(static_cast<REAL>(rcText.left), rcTab.top + (rcTab.Height() - iconH) / 2.0f,
                        static_cast<REAL>(iconW), static_cast<REAL>(iconH));
        const bool on = (m_ghostMark != GhostMark::PinnedIdle);
        const bool pinned = (m_ghostMark != GhostMark::Source);
        DrawGhostIcon(g, box, on ? kGhostOnColor : kGhostIdleColor, pinned, static_cast<REAL>(Scale(2)) * 0.6f);
        rcText.left += iconW + Scale(kGhostIconGap);
    }

    CFont* oldFont = (m_hFont != nullptr) ? dc.SelectObject(CFont::FromHandle(m_hFont)) : nullptr;
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(active ? kActiveTextColor : kInactiveTextColor);
    dc.DrawText(tab.title, &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (oldFont != nullptr)
    {
        dc.SelectObject(oldFont);
    }

    if (glyph == Glyph::Close)
    {
        if (m_hover.tab == index && m_hover.close)
        {
            SolidBrush hoverBrush(kButtonHoverColor);
            g.FillEllipse(&hoverBrush, static_cast<REAL>(rcClose.left), static_cast<REAL>(rcClose.top),
                          static_cast<REAL>(rcClose.Width()), static_cast<REAL>(rcClose.Height()));
        }
        DrawCross(g, rcClose, static_cast<REAL>(Scale(2)) * 0.7f);
    }
    else if (glyph == Glyph::Modified)
    {
        SolidBrush dot(kGlyphColor);
        const REAL d = static_cast<REAL>(Scale(8));
        g.FillEllipse(&dot, rcClose.left + (rcClose.Width() - d) / 2, rcClose.top + (rcClose.Height() - d) / 2, d, d);
    }
}

// ---------------------------------------------------------------------------
// 마우스
// ---------------------------------------------------------------------------

void CTabBar::Notify(TabBarAction action, int index)
{
    GetParent()->SendMessage(WM_TABBAR_ACTION, static_cast<WPARAM>(action), static_cast<LPARAM>(index));
}

void CTabBar::SetHover(const HitResult& hit)
{
    if (hit.tab != m_hover.tab || hit.close != m_hover.close || hit.newTab != m_hover.newTab)
    {
        m_hover = hit;
        Invalidate(FALSE);
    }
}

void CTabBar::OnLButtonDown(UINT, CPoint point)
{
    const HitResult hit = HitTest(point);
    if (hit.newTab)
    {
        Notify(TabBarAction::NewTab, -1);
    }
    else if (hit.close)
    {
        // 닫기는 크롬처럼 버튼을 뗄 때 같은 × 위에 있을 때만
        m_pressedClose = hit.tab;
        SetCapture();
    }
    else if (hit.tab >= 0)
    {
        Notify(TabBarAction::Select, hit.tab);
    }
}

void CTabBar::OnLButtonUp(UINT, CPoint point)
{
    if (m_pressedClose < 0)
    {
        return;
    }
    const int pressed = m_pressedClose;
    m_pressedClose = -1;
    ReleaseCapture();

    const HitResult hit = HitTest(point);
    if (hit.close && hit.tab == pressed)
    {
        Notify(TabBarAction::Close, pressed);
    }
}

void CTabBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    const HitResult hit = HitTest(point);
    if (hit.tab < 0)
    {
        Notify(TabBarAction::NewTab, -1); // + 단추를 두 번 누르거나 빈 곳을 두 번 클릭
    }
    else
    {
        OnLButtonDown(nFlags, point); // 탭 위에서는 보통 클릭과 같게
    }
}

void CTabBar::OnMButtonUp(UINT, CPoint point)
{
    const HitResult hit = HitTest(point);
    if (hit.tab >= 0)
    {
        Notify(TabBarAction::Close, hit.tab); // 가운데 버튼으로 탭 닫기 (크롬과 같음)
    }
}

void CTabBar::OnMouseMove(UINT, CPoint point)
{
    if (!m_trackingMouse)
    {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = GetSafeHwnd();
        m_trackingMouse = ::TrackMouseEvent(&tme) != FALSE;
    }
    SetHover(HitTest(point));
}

void CTabBar::OnMouseLeave()
{
    m_trackingMouse = false;
    SetHover(HitResult{});
}

void CTabBar::OnCaptureChanged(CWnd* pWnd)
{
    if (pWnd != this)
    {
        m_pressedClose = -1;
    }
    CWnd::OnCaptureChanged(pWnd);
}

LRESULT CTabBar::OnSetFont(WPARAM wParam, LPARAM lParam)
{
    m_hFont = reinterpret_cast<HFONT>(wParam);
    if (LOWORD(lParam) != 0)
    {
        Invalidate(FALSE);
    }
    return 0;
}

LRESULT CTabBar::OnGetFont(WPARAM, LPARAM)
{
    return reinterpret_cast<LRESULT>(m_hFont);
}
