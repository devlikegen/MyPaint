/**
 * @file PaintTools.cpp
 * @brief 그리기 도구 구현
 */
#include "pch.h"
#include "PaintTools.h"
#include "CanvasSurface.h"

#include <cmath>
#include <cstdlib>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace Gdiplus;

// ---------------------------------------------------------------------------
// 도구 표시 정보
// ---------------------------------------------------------------------------

const ToolInfo& GetToolInfo(ToolType tool)
{
    static const ToolInfo kInfos[kToolCount] = {
        { L"연필",     L"연필: 끌어서 자유롭게 그립니다. 왼쪽 버튼은 색 1, 오른쪽 버튼은 색 2로 그립니다.", true,  false, 1 },
        { L"브러시",   L"브러시: 부드러운 선으로 그립니다. 왼쪽 버튼은 색 1, 오른쪽 버튼은 색 2로 그립니다.", true,  false, 8 },
        { L"지우개",   L"지우개: 끌어서 색 2로 지웁니다.", true,  false, 12 },
        { L"채우기",   L"채우기: 클릭한 곳과 같은 색으로 이어진 영역을 채웁니다.", false, false, 1 },
        { L"스포이트", L"스포이트: 클릭한 곳의 색을 집습니다. 왼쪽 버튼은 색 1, 오른쪽 버튼은 색 2로 가져옵니다.", false, false, 1 },
        { L"직선",     L"직선: 끌어서 그립니다. Shift를 누르면 45° 단위로 고정됩니다. Esc로 취소합니다.", true,  false, 3 },
        { L"사각형",   L"사각형: 끌어서 그립니다. Shift를 누르면 정사각형이 됩니다. Esc로 취소합니다.", true,  true,  3 },
        { L"타원",     L"타원: 끌어서 그립니다. Shift를 누르면 원이 됩니다. Esc로 취소합니다.", true,  true,  3 },
    };
    const int index = ToIndex(tool);
    ASSERT(index >= 0 && index < kToolCount);
    return kInfos[index];
}

// ---------------------------------------------------------------------------
// 공통 도우미
// ---------------------------------------------------------------------------

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    /**
     * @brief 채우기 색 허용 오차 (채널별 0~255).
     * 거의 같은 색(앤티에일리어싱의 아주 옅은 픽셀)까지 채우되, 흰색(255)과 밝은 회색(192)처럼
     * 실제로 다른 색은 넘지 않는 값입니다. 더 진한 경계 픽셀은 FloodFill의 경계 다듬기가 처리합니다.
     */
    constexpr int kFillTolerance = 48;

    Color ToGdiplusColor(COLORREF color)
    {
        return Color(255, GetRValue(color), GetGValue(color), GetBValue(color));
    }

    /** @brief 두 점을 잇는 선분이 굵기 width로 칠할 수 있는 영역 (여유 포함) */
    CRect SegmentBounds(CPoint a, CPoint b, int width)
    {
        CRect rc(min(a.x, b.x), min(a.y, b.y), max(a.x, b.x) + 1, max(a.y, b.y) + 1);
        rc.InflateRect(width / 2 + 2, width / 2 + 2);
        return rc;
    }

    /** @brief 시작점 기준으로 끝점을 0°, 45°, 90° ... 방향에 맞춥니다. */
    CPoint SnapToAngle(CPoint start, CPoint pt)
    {
        const double dx = pt.x - start.x;
        const double dy = pt.y - start.y;
        if (dx == 0 && dy == 0)
        {
            return pt;
        }
        const double step = kPi / 4;
        const double angle = std::round(std::atan2(dy, dx) / step) * step;
        const double ux = std::cos(angle);
        const double uy = std::sin(angle);
        const double length = dx * ux + dy * uy; // 마우스 위치를 그 방향으로 투영
        return CPoint(start.x + static_cast<int>(std::lround(length * ux)),
                      start.y + static_cast<int>(std::lround(length * uy)));
    }

    /** @brief 시작점 기준으로 끝점을 정사각형이 되게 맞춥니다. */
    CPoint SnapToSquare(CPoint start, CPoint pt)
    {
        const int dx = pt.x - start.x;
        const int dy = pt.y - start.y;
        const int side = max(std::abs(dx), std::abs(dy));
        return CPoint(start.x + (dx < 0 ? -side : side), start.y + (dy < 0 ? -side : side));
    }
}

// ---------------------------------------------------------------------------
// CPaintTool 기본 동작
// ---------------------------------------------------------------------------

void CPaintTool::Drag(IToolHost&, CPoint, bool) {}
void CPaintTool::End(IToolHost&, CPoint, bool) {}
void CPaintTool::Reset() {}
void CPaintTool::DrawOverlay(Graphics&) const {}
void CPaintTool::DrawCursor(Graphics&, CPoint, int) const {}

namespace
{
    // -----------------------------------------------------------------------
    // 자유 곡선 도구: 마우스가 움직일 때마다 직전 위치와 잇는 선분을 바로 그림에 그립니다.
    // -----------------------------------------------------------------------
    class CFreehandTool : public CPaintTool
    {
    public:
        void Begin(IToolHost& host, CPoint pt, const StrokeParams& params) override
        {
            m_params = params;
            m_last = pt;
            Stroke(host, pt, pt);
        }

        void Drag(IToolHost& host, CPoint pt, bool) override
        {
            if (pt != m_last)
            {
                Stroke(host, m_last, pt);
                m_last = pt;
            }
        }

        void End(IToolHost& host, CPoint pt, bool constrain) override
        {
            Drag(host, pt, constrain);
        }

    protected:
        /**
         * @brief 선분 하나를 그립니다. a == b이면 점 하나를 찍습니다.
         * @return 바뀐 영역
         */
        virtual CRect DrawSegment(CDC& dc, CPoint a, CPoint b) = 0;

        StrokeParams m_params;

    private:
        void Stroke(IToolHost& host, CPoint a, CPoint b)
        {
            host.InvalidateImage(DrawSegment(host.Surface().GetDC(), a, b));
        }

        CPoint m_last;
    };

    /** @brief 연필: 앤티에일리어싱 없는 GDI 선 */
    class CPencilTool final : public CFreehandTool
    {
    protected:
        CRect DrawSegment(CDC& dc, CPoint a, CPoint b) override
        {
            const int width = max(1, m_params.size);
            const COLORREF color = m_params.fore;

            if (width == 1)
            {
                CPen pen(PS_SOLID, 1, color);
                CPen* oldPen = dc.SelectObject(&pen);
                dc.MoveTo(a);
                dc.LineTo(b);
                dc.SetPixelV(b, color); // LineTo는 끝점을 칠하지 않음
                dc.SelectObject(oldPen);
            }
            else if (a == b)
            {
                // 길이 0인 선은 그려지지 않으므로 점을 직접 찍음
                CBrush brush(color);
                CBrush* oldBrush = dc.SelectObject(&brush);
                CGdiObject* oldPen = dc.SelectStockObject(NULL_PEN);
                const int half = width / 2;
                // NULL_PEN이면 오른쪽/아래 경계가 빠지므로 1 크게
                dc.Ellipse(a.x - half, a.y - half, a.x - half + width + 1, a.y - half + width + 1);
                dc.SelectObject(oldPen);
                dc.SelectObject(oldBrush);
            }
            else
            {
                LOGBRUSH lb{ BS_SOLID, color, 0 };
                CPen pen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, width, &lb);
                CPen* oldPen = dc.SelectObject(&pen);
                dc.MoveTo(a);
                dc.LineTo(b);
                dc.SelectObject(oldPen);
            }
            return SegmentBounds(a, b, width);
        }
    };

    /** @brief 브러시: GDI+ 앤티에일리어싱 둥근 선 */
    class CBrushTool final : public CFreehandTool
    {
    protected:
        CRect DrawSegment(CDC& dc, CPoint a, CPoint b) override
        {
            const int width = max(1, m_params.size);
            const Color color = ToGdiplusColor(m_params.fore);

            Graphics g(dc.GetSafeHdc());
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            if (a == b)
            {
                SolidBrush brush(color);
                const REAL r = width / 2.0f;
                g.FillEllipse(&brush, a.x - r, a.y - r, 2 * r, 2 * r);
            }
            else
            {
                Pen pen(color, static_cast<REAL>(width));
                pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
                pen.SetLineJoin(LineJoinRound);
                g.DrawLine(&pen, PointF(static_cast<REAL>(a.x), static_cast<REAL>(a.y)), PointF(static_cast<REAL>(b.x), static_cast<REAL>(b.y)));
            }
            return SegmentBounds(a, b, width);
        }
    };

    /** @brief 지우개: 가로세로에 맞춘 정사각형을 경로를 따라 찍음 */
    class CEraserTool final : public CFreehandTool
    {
    public:
        bool HasCustomCursor() const override { return true; }

        void DrawCursor(Graphics& g, CPoint pt, int size) const override
        {
            const CRect rc = StampRect(pt, size);
            g.SetSmoothingMode(SmoothingModeNone);
            Pen pen(Color(255, 0, 0, 0), 1.0f);
            // 픽셀 중심이 정수 좌표이므로 바깥 픽셀 줄을 따라 그림
            g.DrawRectangle(&pen, static_cast<REAL>(rc.left), static_cast<REAL>(rc.top),
                            static_cast<REAL>(max(0, rc.Width() - 1)), static_cast<REAL>(max(0, rc.Height() - 1)));
        }

    protected:
        CRect DrawSegment(CDC& dc, CPoint a, CPoint b) override
        {
            const int width = max(1, m_params.size);
            // 지우개는 어느 버튼이든 색 2(배경색)로 지웁니다.
            const COLORREF color = (m_params.button == MouseButton::Left) ? m_params.back : m_params.fore;

            const int dx = b.x - a.x;
            const int dy = b.y - a.y;
            const int steps = max(std::abs(dx), std::abs(dy));
            for (int i = 0; i <= steps; ++i)
            {
                CPoint p = a;
                if (steps > 0)
                {
                    p.x += static_cast<int>(std::lround(static_cast<double>(dx) * i / steps));
                    p.y += static_cast<int>(std::lround(static_cast<double>(dy) * i / steps));
                }
                dc.FillSolidRect(StampRect(p, width), color);
            }
            return SegmentBounds(a, b, width);
        }

    private:
        static CRect StampRect(CPoint center, int size)
        {
            const int width = max(1, size);
            const int left = center.x - width / 2;
            const int top = center.y - width / 2;
            return CRect(left, top, left + width, top + width);
        }
    };

    // -----------------------------------------------------------------------
    // 한 번 누르면 끝나는 도구
    // -----------------------------------------------------------------------

    /** @brief 채우기 (페인트 통) */
    class CFillTool final : public CPaintTool
    {
    public:
        void Begin(IToolHost& host, CPoint pt, const StrokeParams& params) override
        {
            const CRect dirty = host.Surface().FloodFill(pt, params.fore, kFillTolerance);
            if (!dirty.IsRectEmpty())
            {
                host.InvalidateImage(dirty);
            }
        }
    };

    /** @brief 스포이트. 버튼을 뗀 위치의 색을 집습니다. */
    class CColorPickerTool final : public CPaintTool
    {
    public:
        void Begin(IToolHost&, CPoint pt, const StrokeParams& params) override
        {
            m_button = params.button;
            m_last = pt;
        }

        void Drag(IToolHost& host, CPoint pt, bool) override
        {
            if (host.Surface().Contains(pt))
            {
                m_last = pt; // 그림 밖으로 나가면 마지막으로 그림 안이던 위치를 씀
            }
        }

        void End(IToolHost& host, CPoint pt, bool constrain) override
        {
            Drag(host, pt, constrain);
            host.NotifyColorPicked(host.Surface().GetPixel(m_last), m_button);
        }

    private:
        MouseButton m_button = MouseButton::Left;
        CPoint m_last;
    };

    // -----------------------------------------------------------------------
    // 도형 도구: 끄는 동안 미리보기만 그리고, 버튼을 떼면 그림에 반영합니다.
    // 미리보기와 반영 모두 같은 Render()를 쓰므로 보이는 그대로 그려집니다.
    // -----------------------------------------------------------------------
    class CShapeTool : public CPaintTool
    {
    public:
        void Begin(IToolHost& host, CPoint pt, const StrokeParams& params) override
        {
            m_params = params;
            m_start = m_end = pt;
            m_active = true;
            host.InvalidateOverlay();
        }

        void Drag(IToolHost& host, CPoint pt, bool constrain) override
        {
            m_end = constrain ? Constrain(m_start, pt) : pt;
            host.InvalidateOverlay();
        }

        void End(IToolHost& host, CPoint pt, bool constrain) override
        {
            Drag(host, pt, constrain);
            m_active = false;
            {
                Graphics g(host.Surface().GetDC().GetSafeHdc());
                Render(g);
            }
            host.InvalidateImage(Bounds());
        }

        void Reset() override
        {
            m_active = false;
        }

        void DrawOverlay(Graphics& g) const override
        {
            if (m_active)
            {
                Render(g);
            }
        }

    protected:
        /** @brief Shift를 눌렀을 때 끝점을 맞춥니다. */
        virtual CPoint Constrain(CPoint start, CPoint pt) const = 0;

        /** @brief 현재 시작점~끝점으로 도형을 그립니다. */
        virtual void Render(Graphics& g) const = 0;

        /** @brief 시작점과 끝점을 양 끝 픽셀로 포함하는 사각형 (right/bottom 포함) */
        CRect InclusiveRect() const
        {
            return CRect(min(m_start.x, m_end.x), min(m_start.y, m_end.y),
                         max(m_start.x, m_end.x), max(m_start.y, m_end.y));
        }

        /** @brief 도형이 칠할 수 있는 영역 */
        CRect Bounds() const
        {
            return SegmentBounds(m_start, m_end, max(1, m_params.size));
        }

        StrokeParams m_params;
        CPoint m_start;
        CPoint m_end;
        bool m_active = false;
    };

    /** @brief 직선 */
    class CLineTool final : public CShapeTool
    {
    protected:
        CPoint Constrain(CPoint start, CPoint pt) const override
        {
            return SnapToAngle(start, pt);
        }

        void Render(Graphics& g) const override
        {
            const REAL width = static_cast<REAL>(max(1, m_params.size));
            const Color color = ToGdiplusColor(m_params.fore);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            if (m_start == m_end)
            {
                SolidBrush brush(color);
                g.FillEllipse(&brush, m_start.x - width / 2, m_start.y - width / 2, width, width);
                return;
            }
            Pen pen(color, width);
            pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
            g.DrawLine(&pen, PointF(static_cast<REAL>(m_start.x), static_cast<REAL>(m_start.y)), PointF(static_cast<REAL>(m_end.x), static_cast<REAL>(m_end.y)));
        }
    };

    /**
     * @brief 사각형과 타원의 공통 부분.
     *
     * 끈 범위의 양 끝 픽셀이 도형의 바깥 경계가 되도록, 선은 경계 안쪽으로 굵어집니다.
     * GDI+에서 픽셀 (x, y)는 (x-0.5, y-0.5)~(x+0.5, y+0.5) 영역입니다.
     */
    class CBoxShapeTool : public CShapeTool
    {
    protected:
        CPoint Constrain(CPoint start, CPoint pt) const override
        {
            return SnapToSquare(start, pt);
        }

        void Render(Graphics& g) const override
        {
            const CRect r = InclusiveRect();
            const REAL width = static_cast<REAL>(max(1, m_params.size));
            const RectF outer(r.left - 0.5f, r.top - 0.5f,
                              static_cast<REAL>(r.Width()) + 1.0f, static_cast<REAL>(r.Height()) + 1.0f);
            const RectF center(outer.X + width / 2, outer.Y + width / 2, outer.Width - width, outer.Height - width);

            const Color fore = ToGdiplusColor(m_params.fore);
            const Color back = ToGdiplusColor(m_params.back);
            g.SetSmoothingMode(SmoothingModeAntiAlias);

            if (m_params.fillMode == ShapeFillMode::FillOnly)
            {
                SolidBrush brush(fore);
                FillShape(g, brush, outer);
                return;
            }

            if (center.Width <= 0 || center.Height <= 0)
            {
                // 도형이 선 굵기보다 작으면 선만으로 꽉 차므로 전체를 선 색으로 칠함
                SolidBrush brush(fore);
                FillShape(g, brush, outer);
                return;
            }

            if (m_params.fillMode == ShapeFillMode::OutlineAndFill)
            {
                SolidBrush brush(back);
                FillShape(g, brush, center); // 선 아래로 들어가도록 선 중심까지 채움
            }
            Pen pen(fore, width);
            DrawShape(g, pen, center);
        }

        virtual void FillShape(Graphics& g, const Brush& brush, const RectF& rc) const = 0;
        virtual void DrawShape(Graphics& g, const Pen& pen, const RectF& rc) const = 0;
    };

    /** @brief 사각형 */
    class CRectangleTool final : public CBoxShapeTool
    {
    protected:
        void FillShape(Graphics& g, const Brush& brush, const RectF& rc) const override
        {
            g.FillRectangle(&brush, rc);
        }
        void DrawShape(Graphics& g, const Pen& pen, const RectF& rc) const override
        {
            g.DrawRectangle(&pen, rc);
        }
    };

    /** @brief 타원 */
    class CEllipseTool final : public CBoxShapeTool
    {
    protected:
        void FillShape(Graphics& g, const Brush& brush, const RectF& rc) const override
        {
            g.FillEllipse(&brush, rc);
        }
        void DrawShape(Graphics& g, const Pen& pen, const RectF& rc) const override
        {
            g.DrawEllipse(&pen, rc);
        }
    };
}

std::unique_ptr<CPaintTool> CreatePaintTool(ToolType tool)
{
    switch (tool)
    {
    case ToolType::Pencil:      return std::make_unique<CPencilTool>();
    case ToolType::Brush:       return std::make_unique<CBrushTool>();
    case ToolType::Eraser:      return std::make_unique<CEraserTool>();
    case ToolType::Fill:        return std::make_unique<CFillTool>();
    case ToolType::ColorPicker: return std::make_unique<CColorPickerTool>();
    case ToolType::Line:        return std::make_unique<CLineTool>();
    case ToolType::Rectangle:   return std::make_unique<CRectangleTool>();
    case ToolType::Ellipse:     return std::make_unique<CEllipseTool>();
    }
    ASSERT(FALSE);
    return nullptr;
}
