#pragma once

/**
 * @file CanvasWnd.h
 * @brief 그림을 보여 주고 마우스로 그리게 하는 캔버스 창
 */

#include "CanvasSurface.h"
#include "PaintDocument.h"
#include "PaintTools.h"

#include <array>
#include <memory>
#include <vector>

/**
 * @brief 캔버스 창.
 *
 * 그림(CCanvasSurface)과 현재 도구, 색, 굵기를 가지고, 마우스 입력을 도구에 전달합니다.
 * 그림은 현재 탭의 문서(CPaintDocument)가 가지고, 캔버스는 그 문서를 가리키기만 합니다.
 * 도구와 색 설정은 캔버스가 가지므로 탭을 바꿔도 유지됩니다.
 * 다른 그림을 고스트로 겹쳐 보여 줄 수 있습니다 (SetGhost, 화면에만 보임).
 * 그림이 창보다 크면 스크롤 막대로 이동합니다. 상태가 바뀌면 부모 창에
 * WM_CANVAS_CURSOR, WM_CANVAS_COLORPICKED, WM_CANVAS_MODIFIED 메시지를 보냅니다.
 */
class CCanvasWnd : public CWnd, private IToolHost
{
public:
    CCanvasWnd();
    ~CCanvasWnd() override;

    /**
     * @brief 창을 만듭니다.
     * @param rect 부모 클라이언트 좌표의 위치
     * @param pParent 부모 창
     * @param nID 컨트롤 ID
     * @return 성공하면 TRUE
     */
    BOOL Create(const CRect& rect, CWnd* pParent, UINT nID);

    /**
     * @brief 보여 주고 그릴 문서를 바꿉니다 (탭 전환).
     *
     * 그리는 중이었다면 취소하고, 이전 문서의 스크롤 위치를 저장한 뒤 새 문서의 위치로 돌아갑니다.
     * 도구, 굵기, 색 같은 설정은 그대로 유지합니다.
     * @param document 새 문서. 캔버스는 소유하지 않으며 nullptr이면 빈 화면
     */
    void SetDocument(CPaintDocument* document);

    /** @brief 현재 문서 (없으면 nullptr) */
    CPaintDocument* GetDocument() const { return m_document; }

    /** @brief 그림 크기(px). 문서가 없으면 0 */
    CSize GetImageSize() const;

    /** @brief 현재 도구를 바꿉니다. 그리는 중이었다면 취소합니다. */
    void SetTool(ToolType tool);
    /** @brief 현재 도구 */
    ToolType GetTool() const { return m_tool; }

    /** @brief 도구별 굵기를 정합니다. */
    void SetToolSize(ToolType tool, int size);
    /** @brief 도구별 굵기 */
    int GetToolSize(ToolType tool) const { return m_toolSizes[ToIndex(tool)]; }

    /** @brief 도형 채우기 방식을 정합니다. */
    void SetFillMode(ShapeFillMode mode) { m_fillMode = mode; }
    /** @brief 도형 채우기 방식 */
    ShapeFillMode GetFillMode() const { return m_fillMode; }

    /**
     * @brief 그리기 색을 정합니다.
     * @param primary 색 1 (왼쪽 버튼)
     * @param secondary 색 2 (오른쪽 버튼, 지우개)
     */
    void SetColors(COLORREF primary, COLORREF secondary);

    /** @brief 진행 중인 그리기를 취소하고 그림을 시작 전 상태로 되돌립니다. */
    void CancelStroke();

    /** @brief 마우스로 그리는 중이면 true */
    bool IsStroking() const { return m_stroking; }

    /**
     * @brief 고스트: 다른 그림을 이 그림 위에 반투명하게 비춰 보여 줍니다 (따라 그리기용).
     *
     * 화면에만 보이고 그림 데이터와 저장 파일에는 들어가지 않습니다. 두 그림은 왼쪽 위를 맞춥니다.
     * 고스트의 흰 부분은 그대로 두고 어두운 부분만 비치도록 곱하기로 섞으므로,
     * 내가 그린 선은 흐려지지 않고 원본 그림만 반투명하게 보입니다.
     * @param source 비출 그림 (캔버스는 소유하지 않음). nullptr이면 끕니다.
     *               그 그림을 가진 문서를 지우기 전에 nullptr로 바꿔야 합니다.
     * @param opacityPercent 원본이 비치는 진하기 (0~100)
     */
    void SetGhost(CCanvasSurface* source, int opacityPercent);

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

private:
    // IToolHost
    CCanvasSurface& Surface() override { return Image(); }
    void InvalidateImage(const CRect& rcImage) override;
    void InvalidateOverlay() override;
    void NotifyColorPicked(COLORREF color, MouseButton button) override;

    /** @brief 현재 문서의 그림. 문서가 없으면 비어 있는(IsValid() == false) 그림 */
    CCanvasSurface& Image();

    /** @brief 그리기를 마친 뒤 그림이 바뀌었으면 문서에 표시하고 부모에 알립니다. */
    void MarkModifiedIfChanged();

    /** @brief 현재 도구 객체 */
    CPaintTool& CurrentTool() { return *m_tools[ToIndex(m_tool)]; }

    /** @brief 그림 (0, 0)이 놓이는 클라이언트 좌표 */
    CPoint ImageOrigin() const;
    /** @brief 클라이언트 좌표 → 그림 좌표 */
    CPoint ClientToImage(CPoint ptClient) const;
    /** @brief 그림 좌표 → 클라이언트 좌표 */
    CRect ImageToClient(const CRect& rcImage) const;

    /** @brief 모니터 DPI에 맞춘 픽셀 수 */
    int Scale(int value) const;

    void BeginStroke(CPoint ptClient, MouseButton button);
    void EndStroke(CPoint ptClient, UINT nFlags);

    /** @brief 마우스가 그림 안이면 도구 커서, 밖이면 화살표를 표시합니다. */
    void ApplyCursor(bool insideImage);

    /** @brief 도구 모양 커서(지우개 사각형)를 표시할 마우스 위치를 갱신합니다. */
    void UpdateHover(CPoint ptClient, bool inside);
    /** @brief 도구 모양 커서가 차지하는 클라이언트 영역 */
    CRect HoverClientRect() const;

    void UpdateScrollBars();
    void ScrollTo(int x, int y);
    void HandleScroll(int bar, UINT nSBCode);

    /** @brief 부모 창에 마우스 위치를 알립니다. */
    void NotifyCursor(CPoint ptImage, bool inside);

    /** @brief 지금 고스트를 겹쳐 그려야 하면 true (자기 자신은 겹치지 않음) */
    bool HasGhost();
    /**
     * @brief 그림의 일부에 고스트를 섞어 그립니다.
     * @param dc 그릴 DC
     * @param rcImage 그릴 부분 (그림 좌표, 그림 안)
     * @param ptDest dc에서 그 부분의 왼쪽 위 (논리 좌표)
     */
    void DrawImageWithGhost(CDC& dc, const CRect& rcImage, CPoint ptDest);

    CPaintDocument* m_document = nullptr; ///< 현재 탭의 문서 (소유하지 않음)
    CCanvasSurface m_noImage;   ///< 문서가 없을 때 대신 쓰는 빈 그림
    CCanvasSurface m_backup;    ///< 그리기 시작 직전의 그림 (취소와 변경 확인용)

    std::array<std::unique_ptr<CPaintTool>, kToolCount> m_tools; ///< 도구 객체 (도구 종류 순서)
    std::array<int, kToolCount> m_toolSizes{};                    ///< 도구별 굵기
    ToolType m_tool = ToolType::Pencil;
    ShapeFillMode m_fillMode = ShapeFillMode::Outline;
    COLORREF m_primary = RGB(0, 0, 0);
    COLORREF m_secondary = RGB(255, 255, 255);

    bool m_stroking = false;                  ///< 마우스로 그리는 중
    MouseButton m_strokeButton = MouseButton::Left;
    CPoint m_lastImagePt;                     ///< 그리는 중 마지막 마우스 위치 (그림 좌표)

    CPoint m_scroll;                          ///< 스크롤 위치 (px)
    bool m_updatingScrollBars = false;        ///< 스크롤 막대 갱신 중 재진입 방지

    bool m_trackingMouse = false;             ///< WM_MOUSELEAVE 추적 중
    bool m_hoverVisible = false;              ///< 도구 모양 커서 표시 중
    CPoint m_hoverImagePt;                    ///< 도구 모양 커서 위치 (그림 좌표)

    CCanvasSurface* m_ghost = nullptr;        ///< 고스트로 비출 그림 (소유하지 않음, 없으면 nullptr)
    int m_ghostOpacity = 0;                   ///< 고스트 진하기(%)
    std::vector<uint32_t> m_ghostPixels;      ///< 고스트를 섞은 픽셀을 만드는 작업 공간 (다시 그릴 때마다 재사용)
};
