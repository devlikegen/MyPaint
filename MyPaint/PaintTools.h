#pragma once

/**
 * @file PaintTools.h
 * @brief 그리기 도구 클래스
 *
 * 도구는 마우스 입력(시작 → 끌기 → 끝)을 받아 그림을 바꿉니다.
 * 캔버스 창은 IToolHost를 통해 그림과 화면 갱신만 도구에 내어 주므로,
 * 도구는 창, 스크롤, 마우스 캡처를 몰라도 됩니다.
 */

#include "PaintTypes.h"
#include <memory>

class CCanvasSurface;

/** @brief 도구가 캔버스에 요청할 수 있는 일 */
class IToolHost
{
public:
    /** @brief 그릴 대상 그림 */
    virtual CCanvasSurface& Surface() = 0;

    /**
     * @brief 그림의 일부가 바뀌었으니 화면을 다시 그려 달라고 요청합니다.
     * @param rcImage 그림 좌표의 바뀐 영역
     */
    virtual void InvalidateImage(const CRect& rcImage) = 0;

    /** @brief 미리보기(아직 그림에 반영 안 된 도형)가 바뀌었음을 알립니다. */
    virtual void InvalidateOverlay() = 0;

    /**
     * @brief 스포이트가 색을 집었음을 알립니다.
     * @param color 집은 색
     * @param button 집을 때 쓴 마우스 버튼
     */
    virtual void NotifyColorPicked(COLORREF color, MouseButton button) = 0;

protected:
    ~IToolHost() = default;
};

/** @brief 한 번의 그리기(마우스 누름 ~ 뗌)에 쓰는 설정 */
struct StrokeParams
{
    MouseButton button = MouseButton::Left; ///< 그린 마우스 버튼
    COLORREF fore = RGB(0, 0, 0);           ///< 그리는 색 (왼쪽 버튼이면 색 1)
    COLORREF back = RGB(255, 255, 255);     ///< 반대 색 (왼쪽 버튼이면 색 2)
    int size = 1;                           ///< 굵기(px)
    ShapeFillMode fillMode = ShapeFillMode::Outline; ///< 도형 채우기 방식
};

/**
 * @brief 그리기 도구의 기본 클래스.
 *
 * 좌표는 모두 그림 좌표입니다. 그리기를 취소하면 캔버스가 그림을 시작 전 상태로 되돌린 뒤
 * Reset()을 부르므로, 도구는 자기 상태만 정리하면 됩니다.
 */
class CPaintTool
{
public:
    virtual ~CPaintTool() = default;

    /**
     * @brief 마우스 버튼을 눌렀을 때
     * @param host 캔버스
     * @param pt 누른 위치 (그림 안)
     * @param params 이번 그리기 설정
     */
    virtual void Begin(IToolHost& host, CPoint pt, const StrokeParams& params) = 0;

    /**
     * @brief 버튼을 누른 채 마우스를 움직였을 때
     * @param constrain Shift를 누르고 있으면 true (직선 각도, 정사각형/원 고정)
     */
    virtual void Drag(IToolHost& host, CPoint pt, bool constrain);

    /** @brief 마우스 버튼을 뗐을 때. 그리기를 마무리합니다. */
    virtual void End(IToolHost& host, CPoint pt, bool constrain);

    /** @brief 그리기가 취소됐을 때 도구 상태를 정리합니다. */
    virtual void Reset();

    /**
     * @brief 그림 위에 미리보기를 그립니다 (예: 끌고 있는 도형).
     * @param g 그림 좌표계로 맞춰진 그래픽스
     */
    virtual void DrawOverlay(Gdiplus::Graphics& g) const;

    /**
     * @brief 마우스 위치에 도구 모양 커서를 그립니다 (예: 지우개 사각형).
     * @param g 그림 좌표계로 맞춰진 그래픽스
     * @param pt 마우스 위치
     * @param size 현재 굵기
     */
    virtual void DrawCursor(Gdiplus::Graphics& g, CPoint pt, int size) const;

    /** @brief DrawCursor로 커서를 직접 그리면 true (시스템 커서를 숨김) */
    virtual bool HasCustomCursor() const { return false; }
};

/**
 * @brief 도구 객체를 만듭니다.
 * @param tool 도구 종류
 */
std::unique_ptr<CPaintTool> CreatePaintTool(ToolType tool);
