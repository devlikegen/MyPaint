/**
 * @file CanvasWnd.cpp
 * @brief 캔버스 창 구현
 */
#include "pch.h"
#include "CanvasWnd.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    constexpr int kImageMargin = 8;     ///< 그림 둘레 여백 (96 DPI 기준 px)
    constexpr int kShadowOffset = 3;    ///< 그림 그림자 거리 (96 DPI 기준 px)
    constexpr int kLineScroll = 24;     ///< 스크롤 화살표 한 번에 움직이는 거리 (96 DPI 기준 px)
    constexpr int kWheelScroll = 72;    ///< 휠 한 칸에 움직이는 거리 (96 DPI 기준 px)
    constexpr int kMaxToolSize = 200;   ///< 굵기 최댓값

    const COLORREF kShadowColor = RGB(171, 182, 199);    ///< 그림 그림자 색
}

BEGIN_MESSAGE_MAP(CCanvasWnd, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_MOUSEHWHEEL()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_CAPTURECHANGED()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CCanvasWnd::CCanvasWnd()
{
    for (int i = 0; i < kToolCount; ++i)
    {
        const ToolType tool = static_cast<ToolType>(i);
        m_tools[i] = CreatePaintTool(tool);
        m_toolSizes[i] = GetToolInfo(tool).defaultSize;
    }
}

CCanvasWnd::~CCanvasWnd() = default;

BOOL CCanvasWnd::Create(const CRect& rect, CWnd* pParent, UINT nID)
{
    const CString className = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return CWnd::Create(className, nullptr, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS,
                        rect, pParent, nID);
}

void CCanvasWnd::SetDocument(CPaintDocument* document)
{
    CancelStroke(); // 되돌릴 그림(m_backup)은 이전 문서의 것이므로 먼저 정리
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE);
        m_hoverVisible = false;
    }
    if (m_document != nullptr)
    {
        m_document->SetScroll(m_scroll);
    }

    m_document = document;
    m_scroll = (m_document != nullptr) ? m_document->GetScroll() : CPoint(0, 0);
    if (GetSafeHwnd() != nullptr)
    {
        UpdateScrollBars(); // 크기가 다른 그림이면 범위가 바뀌고, 위치도 범위 안으로 맞춰짐
        Invalidate(FALSE);
    }
}

CSize CCanvasWnd::GetImageSize() const
{
    return (m_document != nullptr) ? m_document->Surface().GetSize() : CSize(0, 0);
}

CCanvasSurface& CCanvasWnd::Image()
{
    return (m_document != nullptr) ? m_document->Surface() : m_noImage;
}

void CCanvasWnd::MarkModifiedIfChanged()
{
    if (m_document == nullptr || Image().ContentEquals(m_backup))
    {
        return; // 스포이트, 같은 색 채우기처럼 바뀐 것이 없으면 그대로
    }
    if (!m_document->IsModified())
    {
        m_document->SetModified(true);
        GetParent()->SendMessage(WM_CANVAS_MODIFIED);
    }
}

void CCanvasWnd::SetTool(ToolType tool)
{
    CancelStroke();
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE);
        m_hoverVisible = false;
    }
    m_tool = tool;
}

void CCanvasWnd::SetToolSize(ToolType tool, int size)
{
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE); // 이전 크기의 커서 지우기
    }
    m_toolSizes[ToIndex(tool)] = max(1, min(size, kMaxToolSize));
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE);
    }
}

void CCanvasWnd::SetColors(COLORREF primary, COLORREF secondary)
{
    m_primary = primary;
    m_secondary = secondary;
}

void CCanvasWnd::SetGhost(CCanvasSurface* source, int opacityPercent)
{
    opacityPercent = max(0, min(opacityPercent, 100));
    if (source == m_ghost && opacityPercent == m_ghostOpacity)
    {
        return;
    }
    m_ghost = source;
    m_ghostOpacity = opacityPercent;
    if (GetSafeHwnd() != nullptr)
    {
        Invalidate(FALSE);
    }
}

void CCanvasWnd::CancelStroke()
{
    if (!m_stroking)
    {
        return;
    }
    m_stroking = false; // 먼저 내려야 캡처 해제 알림에서 그리기를 마무리하지 않음
    CurrentTool().Reset();
    Image().CopyFrom(m_backup);
    if (GetCapture() == this)
    {
        ReleaseCapture();
    }
    Invalidate(FALSE);
}

// ---------------------------------------------------------------------------
// IToolHost
// ---------------------------------------------------------------------------

void CCanvasWnd::InvalidateImage(const CRect& rcImage)
{
    InvalidateRect(ImageToClient(rcImage), FALSE);
}

void CCanvasWnd::InvalidateOverlay()
{
    // 이전 미리보기가 어디까지였는지 모르므로 전체를 다시 그림 (이중 버퍼라 깜빡이지 않음)
    Invalidate(FALSE);
}

void CCanvasWnd::NotifyColorPicked(COLORREF color, MouseButton button)
{
    GetParent()->SendMessage(WM_CANVAS_COLORPICKED, static_cast<WPARAM>(button), static_cast<LPARAM>(color));
}

// ---------------------------------------------------------------------------
// 좌표 변환
// ---------------------------------------------------------------------------

int CCanvasWnd::Scale(int value) const
{
    const UINT dpi = (GetSafeHwnd() != nullptr) ? ::GetDpiForWindow(GetSafeHwnd()) : USER_DEFAULT_SCREEN_DPI;
    return ::MulDiv(value, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
}

CPoint CCanvasWnd::ImageOrigin() const
{
    const int margin = Scale(kImageMargin);
    return CPoint(margin - m_scroll.x, margin - m_scroll.y);
}

CPoint CCanvasWnd::ClientToImage(CPoint ptClient) const
{
    return ptClient - ImageOrigin();
}

CRect CCanvasWnd::ImageToClient(const CRect& rcImage) const
{
    CRect rc(rcImage);
    rc.OffsetRect(ImageOrigin());
    return rc;
}

// ---------------------------------------------------------------------------
// 그리기
// ---------------------------------------------------------------------------

BOOL CCanvasWnd::OnEraseBkgnd(CDC*)
{
    return TRUE; // OnPaint에서 배경까지 모두 그림
}

void CCanvasWnd::OnPaint()
{
    CPaintDC dc(this);
    const CRect rcPaint(dc.m_ps.rcPaint);
    if (rcPaint.IsRectEmpty())
    {
        return;
    }

    // 다시 그릴 영역만큼의 이중 버퍼
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap buffer;
    buffer.CreateCompatibleBitmap(&dc, rcPaint.Width(), rcPaint.Height());
    CBitmap* oldBitmap = memDC.SelectObject(&buffer);
    memDC.SetViewportOrg(-rcPaint.left, -rcPaint.top);

    memDC.FillSolidRect(rcPaint, kCanvasWorkspaceColor);

    if (Image().IsValid())
    {
        const CPoint origin = ImageOrigin();
        const CSize size = Image().GetSize();
        const CRect rcImage(origin, size);

        CRect rcShadow(rcImage);
        rcShadow.OffsetRect(Scale(kShadowOffset), Scale(kShadowOffset));
        memDC.FillSolidRect(rcShadow, kShadowColor);

        CRect rcBlit;
        if (rcBlit.IntersectRect(rcImage, rcPaint))
        {
            CRect rcSource(rcBlit);
            rcSource.OffsetRect(-origin); // 그림 좌표
            if (HasGhost())
            {
                DrawImageWithGhost(memDC, rcSource, rcBlit.TopLeft());
            }
            else
            {
                memDC.BitBlt(rcBlit.left, rcBlit.top, rcBlit.Width(), rcBlit.Height(), &Image().GetDC(),
                             rcSource.left, rcSource.top, SRCCOPY);
            }
        }

        // 미리보기와 도구 커서는 그림 좌표로 그림
        memDC.SetViewportOrg(0, 0);
        {
            Gdiplus::Graphics g(memDC.GetSafeHdc());
            g.TranslateTransform(static_cast<Gdiplus::REAL>(origin.x - rcPaint.left),
                                 static_cast<Gdiplus::REAL>(origin.y - rcPaint.top));
            g.SetClip(Gdiplus::Rect(0, 0, size.cx, size.cy));

            CPaintTool& tool = CurrentTool();
            tool.DrawOverlay(g);
            if (m_hoverVisible && tool.HasCustomCursor())
            {
                tool.DrawCursor(g, m_hoverImagePt, GetToolSize(m_tool));
            }
        }
    }

    memDC.SetViewportOrg(0, 0);
    dc.BitBlt(rcPaint.left, rcPaint.top, rcPaint.Width(), rcPaint.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(oldBitmap);
}

bool CCanvasWnd::HasGhost()
{
    return m_ghost != nullptr && m_ghostOpacity > 0 && m_ghost != &Image() && m_ghost->IsValid() && Image().IsValid();
}

void CCanvasWnd::DrawImageWithGhost(CDC& dc, const CRect& rcImage, CPoint ptDest)
{
    CCanvasSurface& image = Image();
    const int width = rcImage.Width();
    const int height = rcImage.Height();
    const int imageWidth = image.GetSize().cx;
    const CSize ghostSize = m_ghost->GetSize();
    const uint32_t* imagePixels = image.Pixels();
    const uint32_t* ghostPixels = m_ghost->Pixels();

    // 고스트 색 값(0~255)마다 그림에 곱할 값: 흰색(255)은 255(그대로), 검정(0)은 255 × (1 - 진하기)
    BYTE factor[256];
    for (int c = 0; c < 256; ++c)
    {
        factor[c] = static_cast<BYTE>(255 - ((255 - c) * m_ghostOpacity + 50) / 100);
    }
    auto multiply = [&factor](uint32_t pixel, uint32_t ghost) -> uint32_t {
        const uint32_t r = (((pixel >> 16) & 0xFF) * factor[(ghost >> 16) & 0xFF] + 127) / 255;
        const uint32_t g = (((pixel >> 8) & 0xFF) * factor[(ghost >> 8) & 0xFF] + 127) / 255;
        const uint32_t b = ((pixel & 0xFF) * factor[ghost & 0xFF] + 127) / 255;
        return (r << 16) | (g << 8) | b;
    };

    m_ghostPixels.resize(static_cast<size_t>(width) * height);
    // 고스트가 덮는 가로 칸 수 (두 그림은 왼쪽 위를 맞추므로, 고스트가 작으면 오른쪽/아래는 그림 그대로)
    const int overlapX = max(0, min(width, ghostSize.cx - rcImage.left));
    for (int y = 0; y < height; ++y)
    {
        const int imageY = rcImage.top + y;
        const uint32_t* src = imagePixels + static_cast<size_t>(imageY) * imageWidth + rcImage.left;
        uint32_t* dst = m_ghostPixels.data() + static_cast<size_t>(y) * width;
        int x = 0;
        if (overlapX > 0 && imageY < ghostSize.cy)
        {
            const uint32_t* ghost = ghostPixels + static_cast<size_t>(imageY) * ghostSize.cx + rcImage.left;
            for (; x < overlapX; ++x)
            {
                dst[x] = multiply(src[x], ghost[x]);
            }
        }
        std::copy(src + x, src + width, dst + x);
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 위에서 아래로
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    ::StretchDIBits(dc.GetSafeHdc(), ptDest.x, ptDest.y, width, height, 0, 0, width, height,
                    m_ghostPixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

// ---------------------------------------------------------------------------
// 스크롤
// ---------------------------------------------------------------------------

void CCanvasWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    UpdateScrollBars();
}

void CCanvasWnd::UpdateScrollBars()
{
    if (m_updatingScrollBars || GetSafeHwnd() == nullptr)
    {
        return;
    }
    m_updatingScrollBars = true;

    const CSize image = Image().GetSize();
    const int margin = Scale(kImageMargin);
    const CSize content(image.cx + 2 * margin, image.cy + 2 * margin);

    // 한쪽 스크롤 막대가 나타나면 다른 쪽 클라이언트 크기가 줄어들므로 두 번 계산
    for (int pass = 0; pass < 2; ++pass)
    {
        CRect rcClient;
        GetClientRect(&rcClient);

        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;

        si.nMax = content.cx - 1;
        si.nPage = static_cast<UINT>(rcClient.Width());
        si.nPos = m_scroll.x;
        SetScrollInfo(SB_HORZ, &si, TRUE);

        si.nMax = content.cy - 1;
        si.nPage = static_cast<UINT>(rcClient.Height());
        si.nPos = m_scroll.y;
        SetScrollInfo(SB_VERT, &si, TRUE);
    }

    // 범위를 벗어난 위치는 시스템이 잘라 주므로 다시 읽음
    m_scroll.x = GetScrollPos(SB_HORZ);
    m_scroll.y = GetScrollPos(SB_VERT);

    m_updatingScrollBars = false;
    Invalidate(FALSE);
}

void CCanvasWnd::ScrollTo(int x, int y)
{
    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;

    GetScrollInfo(SB_HORZ, &si);
    const int maxX = max(0, si.nMax - static_cast<int>(si.nPage) + 1);
    GetScrollInfo(SB_VERT, &si);
    const int maxY = max(0, si.nMax - static_cast<int>(si.nPage) + 1);

    const CPoint target(max(0, min(x, maxX)), max(0, min(y, maxY)));
    if (target == m_scroll)
    {
        return;
    }
    m_scroll = target;
    SetScrollPos(SB_HORZ, m_scroll.x, TRUE);
    SetScrollPos(SB_VERT, m_scroll.y, TRUE);
    Invalidate(FALSE);
}

void CCanvasWnd::HandleScroll(int bar, UINT nSBCode)
{
    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(bar, &si);

    int pos = si.nPos;
    switch (nSBCode)
    {
    case SB_LINEUP:        pos -= Scale(kLineScroll); break;
    case SB_LINEDOWN:      pos += Scale(kLineScroll); break;
    case SB_PAGEUP:        pos -= static_cast<int>(si.nPage); break;
    case SB_PAGEDOWN:      pos += static_cast<int>(si.nPage); break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: pos = si.nTrackPos; break;
    case SB_TOP:           pos = si.nMin; break;
    case SB_BOTTOM:        pos = si.nMax; break;
    default:               return;
    }

    if (bar == SB_HORZ)
    {
        ScrollTo(pos, m_scroll.y);
    }
    else
    {
        ScrollTo(m_scroll.x, pos);
    }
}

void CCanvasWnd::OnHScroll(UINT nSBCode, UINT, CScrollBar*)
{
    HandleScroll(SB_HORZ, nSBCode);
}

void CCanvasWnd::OnVScroll(UINT nSBCode, UINT, CScrollBar*)
{
    HandleScroll(SB_VERT, nSBCode);
}

BOOL CCanvasWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint)
{
    const int amount = ::MulDiv(-zDelta, Scale(kWheelScroll), WHEEL_DELTA);
    if (nFlags & MK_SHIFT)
    {
        ScrollTo(m_scroll.x + amount, m_scroll.y); // Shift + 휠: 가로 스크롤
    }
    else
    {
        ScrollTo(m_scroll.x, m_scroll.y + amount);
    }
    return TRUE;
}

void CCanvasWnd::OnMouseHWheel(UINT, short zDelta, CPoint)
{
    ScrollTo(m_scroll.x + ::MulDiv(zDelta, Scale(kWheelScroll), WHEEL_DELTA), m_scroll.y);
}

// ---------------------------------------------------------------------------
// 마우스
// ---------------------------------------------------------------------------

void CCanvasWnd::BeginStroke(CPoint ptClient, MouseButton button)
{
    SetFocus();

    const CPoint pt = ClientToImage(ptClient);
    if (!Image().IsValid() || !Image().Contains(pt))
    {
        return; // 그림 밖에서는 그리기를 시작하지 않음
    }
    if (!m_backup.CopyFrom(Image()))
    {
        return; // 되돌릴 수 없으면 그리지 않음
    }

    m_stroking = true;
    m_strokeButton = button;
    m_lastImagePt = pt;
    SetCapture();

    StrokeParams params;
    params.button = button;
    params.fore = (button == MouseButton::Left) ? m_primary : m_secondary;
    params.back = (button == MouseButton::Left) ? m_secondary : m_primary;
    params.size = GetToolSize(m_tool);
    params.fillMode = m_fillMode;
    CurrentTool().Begin(*this, pt, params);
}

void CCanvasWnd::EndStroke(CPoint ptClient, UINT nFlags)
{
    const CPoint pt = ClientToImage(ptClient);
    m_stroking = false; // 먼저 내려야 캡처 해제 알림에서 두 번 마무리하지 않음
    CurrentTool().End(*this, pt, (nFlags & MK_SHIFT) != 0);
    if (GetCapture() == this)
    {
        ReleaseCapture();
    }
    MarkModifiedIfChanged();
}

void CCanvasWnd::OnLButtonDown(UINT, CPoint point)
{
    if (m_stroking)
    {
        // 오른쪽 버튼으로 그리는 중에 왼쪽 버튼을 누르면 취소 (그림판과 같음)
        if (m_strokeButton != MouseButton::Left)
        {
            CancelStroke();
        }
        return;
    }
    BeginStroke(point, MouseButton::Left);
}

void CCanvasWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_stroking && m_strokeButton == MouseButton::Left)
    {
        EndStroke(point, nFlags);
    }
}

void CCanvasWnd::OnRButtonDown(UINT, CPoint point)
{
    if (m_stroking)
    {
        if (m_strokeButton != MouseButton::Right)
        {
            CancelStroke();
        }
        return;
    }
    BeginStroke(point, MouseButton::Right);
}

void CCanvasWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
    // 기본 처리를 하지 않아 WM_CONTEXTMENU가 생기지 않게 함
    if (m_stroking && m_strokeButton == MouseButton::Right)
    {
        EndStroke(point, nFlags);
    }
}

void CCanvasWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_trackingMouse)
    {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = GetSafeHwnd();
        m_trackingMouse = ::TrackMouseEvent(&tme) != FALSE;
    }

    const CPoint pt = ClientToImage(point);
    const bool inside = Image().Contains(pt);
    NotifyCursor(pt, inside);
    UpdateHover(point, inside);

    if (m_stroking)
    {
        ApplyCursor(inside); // 캡처 중에는 WM_SETCURSOR가 오지 않음
        m_lastImagePt = pt;
        CurrentTool().Drag(*this, pt, (nFlags & MK_SHIFT) != 0);
    }
}

void CCanvasWnd::OnMouseLeave()
{
    m_trackingMouse = false;
    NotifyCursor(CPoint(0, 0), false);
    UpdateHover(CPoint(0, 0), false);
}

void CCanvasWnd::OnCaptureChanged(CWnd* pWnd)
{
    // 다른 창이 캡처를 가져가면(예: Alt+Tab) 마지막 위치에서 그리기를 마무리
    if (m_stroking && pWnd != this)
    {
        m_stroking = false;
        CurrentTool().End(*this, m_lastImagePt, false);
        MarkModifiedIfChanged();
    }
    CWnd::OnCaptureChanged(pWnd);
}

BOOL CCanvasWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT)
    {
        CPoint pt;
        ::GetCursorPos(&pt);
        ScreenToClient(&pt);
        ApplyCursor(Image().Contains(ClientToImage(pt)));
        return TRUE;
    }
    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CCanvasWnd::ApplyCursor(bool insideImage)
{
    if (!insideImage)
    {
        ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    }
    else if (CurrentTool().HasCustomCursor())
    {
        ::SetCursor(nullptr); // 도구가 직접 그린 커서만 보이게
    }
    else
    {
        ::SetCursor(::LoadCursor(nullptr, IDC_CROSS));
    }
}

void CCanvasWnd::UpdateHover(CPoint ptClient, bool inside)
{
    const bool visible = inside && CurrentTool().HasCustomCursor();
    const CPoint pt = ClientToImage(ptClient);
    if (visible == m_hoverVisible && (!visible || pt == m_hoverImagePt))
    {
        return;
    }
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE);
    }
    m_hoverVisible = visible;
    m_hoverImagePt = pt;
    if (m_hoverVisible)
    {
        InvalidateRect(HoverClientRect(), FALSE);
    }
}

CRect CCanvasWnd::HoverClientRect() const
{
    const int size = GetToolSize(m_tool);
    CRect rc(m_hoverImagePt.x - size / 2, m_hoverImagePt.y - size / 2,
             m_hoverImagePt.x - size / 2 + size, m_hoverImagePt.y - size / 2 + size);
    rc.InflateRect(2, 2);
    return ImageToClient(rc);
}

void CCanvasWnd::NotifyCursor(CPoint ptImage, bool inside)
{
    GetParent()->SendMessage(WM_CANVAS_CURSOR, inside ? 1 : 0, MAKELPARAM(ptImage.x, ptImage.y));
}
