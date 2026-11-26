#pragma once

/**
 * @file PaintTypes.h
 * @brief 그리기 기능 전반에서 함께 쓰는 타입과 메시지 정의
 */

/** @brief 그리기 도구 종류. 값은 도구 버튼 순서와 같습니다. */
enum class ToolType
{
    Pencil,      ///< 연필: 앤티에일리어싱 없는 단단한 선
    Brush,       ///< 브러시: 부드러운(앤티에일리어싱) 둥근 선
    Eraser,      ///< 지우개: 색 2로 사각형 도장을 찍음
    Fill,        ///< 채우기(페인트 통): 같은 색 영역을 채움
    ColorPicker, ///< 스포이트: 캔버스에서 색을 집음
    Line,        ///< 직선
    Rectangle,   ///< 사각형
    Ellipse,     ///< 타원
};

/** @brief 그림 바깥 작업 영역 색. 선택된 탭도 이 색으로 칠해 캔버스와 이어 보이게 합니다. */
constexpr COLORREF kCanvasWorkspaceColor = RGB(201, 211, 226);

/** @brief 도구 개수 */
constexpr int kToolCount = 8;

/** @brief 도구를 배열 인덱스로 바꿉니다. */
constexpr int ToIndex(ToolType tool) { return static_cast<int>(tool); }

/** @brief 도형(사각형, 타원)을 그리는 방식 */
enum class ShapeFillMode
{
    Outline,        ///< 윤곽선만 (그린 색)
    OutlineAndFill, ///< 윤곽선(그린 색) + 채우기(반대 색)
    FillOnly,       ///< 채우기만 (그린 색)
};

/** @brief 그리기에 쓴 마우스 버튼. 왼쪽은 색 1, 오른쪽은 색 2로 그립니다. */
enum class MouseButton
{
    Left,
    Right,
};

/** @brief 도구 하나에 대한 표시 정보 */
struct ToolInfo
{
    const wchar_t* name;  ///< 버튼에 표시할 이름
    const wchar_t* hint;  ///< 상태 표시줄에 표시할 사용법
    bool usesSize;        ///< 굵기 설정을 쓰는지
    bool usesFillMode;    ///< 도형 채우기 설정을 쓰는지
    int defaultSize;      ///< 기본 굵기(px)
};

/**
 * @brief 도구의 표시 정보를 돌려줍니다.
 * @param tool 도구 종류
 * @return 정적 수명의 도구 정보
 */
const ToolInfo& GetToolInfo(ToolType tool);

/**
 * @brief 캔버스 → 부모: 마우스 위치가 바뀜.
 * wParam: 이미지 안이면 1, 밖이면 0. lParam: 이미지 좌표 (GET_X_LPARAM / GET_Y_LPARAM)
 */
constexpr UINT WM_CANVAS_CURSOR = WM_APP + 1;

/**
 * @brief 캔버스 → 부모: 스포이트로 색을 집음.
 * wParam: MouseButton, lParam: COLORREF
 */
constexpr UINT WM_CANVAS_COLORPICKED = WM_APP + 2;

/**
 * @brief 팔레트 → 부모: 사용자가 색 1 또는 색 2를 바꿈.
 * wParam, lParam 없음. 팔레트에서 현재 색을 읽으면 됩니다.
 */
constexpr UINT WM_PALETTE_COLORCHANGED = WM_APP + 3;

/**
 * @brief 캔버스 → 부모: 현재 문서가 저장된 상태에서 처음으로 바뀜 (탭 제목에 표시하도록).
 * wParam, lParam 없음.
 */
constexpr UINT WM_CANVAS_MODIFIED = WM_APP + 4;

/**
 * @brief 탭 표시줄 → 부모: 사용자가 탭을 조작함.
 * wParam: TabBarAction, lParam: 대상 탭 번호 (NewTab이면 -1)
 */
constexpr UINT WM_TABBAR_ACTION = WM_APP + 5;

/** @brief 탭 표시줄에서 일어난 조작 */
enum class TabBarAction
{
    Select,  ///< 탭을 클릭함
    Close,   ///< 탭의 × 단추를 누르거나 가운데 버튼으로 클릭함
    NewTab,  ///< + 단추를 누르거나 빈 곳을 두 번 클릭함
};
