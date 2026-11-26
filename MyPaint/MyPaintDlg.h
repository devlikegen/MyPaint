#pragma once

/**
 * @file MyPaintDlg.h
 * @brief 주 대화 상자 클래스 선언
 */

#include "CanvasWnd.h"
#include "ColorPalette.h"
#include "PaintDocument.h"
#include "TabBar.h"

#include <array>
#include <memory>
#include <vector>

/**
 * @brief 주 대화 상자.
 *
 * 위쪽 도구 모음(파일 / 도구 / 모양 / 색 / 고스트 트레이싱), 탭 표시줄, 캔버스, 아래쪽 상태 표시줄로 구성됩니다.
 * 컨트롤은 DPI에 맞춰 코드에서 만들고 배치합니다.
 *
 * 탭마다 문서(CPaintDocument)가 하나씩 있고, 캔버스는 선택된 탭의 문서만 보여 줍니다.
 * 도구 모음의 설정(도구, 굵기, 채우기, 색)은 캔버스가 가지므로 모든 탭이 공유합니다.
 * 파일 묶음의 단추와 단축키는 같은 동작을 합니다:
 *   Ctrl+T 새 탭, Ctrl+O 열기, Ctrl+S 저장, Ctrl+Shift+S 다른 이름으로 저장,
 *   Ctrl+W 탭 닫기, Ctrl+Tab / Ctrl+Shift+Tab 다음/이전 탭
 *
 * 고스트 트레이싱: 다른 탭의 그림을 지금 탭에 반투명하게 비춰, 그 위에 대고 따라 그립니다 (화면에만 보임).
 *   Ctrl+G 켜기/끄기, Ctrl+Shift+G 지금 탭을 원본으로 고정/해제 (고정이 없으면 바로 왼쪽 탭을 비춤),
 *   진하기 25% / 50% / 75%. 원본 탭 자신을 볼 때는 겹치지 않습니다.
 *   켜짐 여부와 원본은 상태 표시줄과 원본 탭 이름 앞의 유령 표시로 알려 줍니다.
 */
class CMyPaintDlg : public CDialog
{
public:
    /**
     * @brief 생성자
     * @param pParent 부모 창 (없으면 nullptr)
     */
    explicit CMyPaintDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MYPAINT_DIALOG };
#endif

protected:
    /**
     * @brief 컨트롤과 멤버 변수 사이의 데이터 교환
     * @param pDX 데이터 교환 개체
     */
    void DoDataExchange(CDataExchange* pDX) override;

    /**
     * @brief 대화 상자 초기화
     * @return 포커스를 컨트롤에 직접 지정하지 않으면 TRUE
     */
    BOOL OnInitDialog() override;

    /** @brief Enter 키로 대화 상자가 닫히지 않게 합니다. */
    void OnOK() override;

    /** @brief Esc 키: 그리는 중이면 취소하고, 대화 상자는 닫지 않습니다. */
    void OnCancel() override;

    /** @brief 단축키(가속기)를 먼저 처리합니다. */
    BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void OnNewTab();
    afx_msg void OnOpenImage();
    afx_msg void OnCloseTab();
    afx_msg void OnSaveTab();
    afx_msg void OnSaveTabAs();
    afx_msg void OnNextTab();
    afx_msg void OnPrevTab();
    afx_msg LRESULT OnTabBarAction(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCanvasModified(WPARAM wParam, LPARAM lParam);
    afx_msg void OnToggleGhost();
    afx_msg void OnPinGhostSource();
    afx_msg void OnGhostOpacity(UINT nID);

    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
    afx_msg void OnToolButton(UINT nID);
    afx_msg void OnSizeComboChanged();
    afx_msg void OnFillComboChanged();
    afx_msg void OnEditColor();
    afx_msg LRESULT OnCanvasCursor(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCanvasColorPicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnPaletteColorChanged(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    /** @brief 도구 모음의 묶음 (왼쪽부터) */
    enum Group
    {
        GroupFile,
        GroupTools,
        GroupShape,
        GroupColors,
        GroupGhost,
        GroupCount,
    };

    /** @brief 현재 DPI에 맞춘 픽셀 수 (96 DPI 기준 값을 받음) */
    int Scale(int value) const;

    /** @brief 글자가 잘리지 않는 단추 너비 (현재 글꼴의 글자 너비 + 양옆 여백) */
    int ButtonTextWidth(const CString& text);

    /** @brief 현재 DPI의 메시지 글꼴을 만들어 모든 컨트롤에 적용합니다. */
    void UpdateUiFont();

    void CreateControls();
    void LayoutControls();

    /** @brief 도구를 고르고 도구 버튼과 옵션 컨트롤을 맞춥니다. */
    void SelectTool(ToolType tool);
    /** @brief 현재 도구에 맞게 굵기, 채우기 컨트롤을 갱신합니다. */
    void UpdateToolOptions();
    /** @brief 팔레트의 색을 캔버스에 반영합니다. */
    void SyncColorsToCanvas();
    void UpdateImageSizeStatus();
    /** @brief 상태 표시줄을 칸으로 나눕니다 (현재 DPI). 칸이 있어야 칸별 글자를 넣을 수 있습니다. */
    void UpdateStatusBarParts();

    // ----- 탭 -----

    /**
     * @brief 빈 그림의 새 탭을 맨 끝에 만들고 선택합니다.
     * @return 새 탭 번호. 실패하면 -1
     */
    int AddNewTab();

    /**
     * @brief 문서를 새 탭으로 맨 끝에 붙입니다. 선택은 바꾸지 않습니다.
     * @return 새 탭 번호
     */
    int AppendDocument(std::unique_ptr<CPaintDocument> document);

    /** @brief 탭의 문서를 다른 문서로 바꿉니다 (빈 탭에 파일을 불러올 때). */
    void ReplaceDocument(int index, std::unique_ptr<CPaintDocument> document);

    /** @brief 같은 파일을 연 탭 번호. 없으면 -1 */
    int FindTabByPath(const CString& path) const;

    /**
     * @brief 이미지 파일들을 엽니다.
     *
     * 이미 열린 파일은 그 탭으로 가고, 지금 탭이 아무것도 안 그린 빈 탭이면 첫 파일을 그 탭에 엽니다.
     * 나머지는 새 탭으로 열고, 마지막으로 연 탭을 선택합니다. 열지 못한 파일은 한꺼번에 알립니다.
     */
    void OpenImageFiles(const std::vector<CString>& paths);

    /**
     * @brief 열기 대화 상자로 열 파일들을 받습니다 (여러 개 선택 가능).
     * @param paths [out] 고른 파일 경로
     * @return 하나 이상 골랐으면 true
     */
    bool AskOpenPaths(std::vector<CString>& paths);

    /** @brief 탭을 선택하고 캔버스에 그 탭의 문서를 보여 줍니다. */
    void ActivateTab(int index);

    /**
     * @brief 탭을 닫습니다. 저장하지 않은 변경이 있으면 먼저 물어봅니다.
     * 마지막 탭을 닫으면 빈 새 탭을 하나 엽니다.
     * @return 닫았으면 true, 사용자가 취소했으면 false
     */
    bool CloseTab(int index);

    /**
     * @brief 탭의 그림을 파일로 저장합니다.
     * @param index 탭 번호
     * @param askPath true이면 저장 경로가 있어도 저장 대화 상자를 띄움 (다른 이름으로 저장)
     * @return 저장했으면 true, 취소했거나 실패했으면 false
     */
    bool SaveTab(int index, bool askPath);

    /**
     * @brief 저장 대화 상자로 저장 경로를 받습니다 (PNG 기본, BMP/JPEG).
     * @param document 저장할 문서 (기본 파일 이름과 폴더를 정하는 데 씀)
     * @param path [out] 고른 경로. 확장자가 없으면 고른 형식의 확장자를 붙임
     * @return 경로를 골랐으면 true
     */
    bool AskSavePath(const CPaintDocument& document, CString& path);

    /**
     * @brief 저장하지 않은 변경이 있으면 저장할지 묻습니다.
     * @return 계속 진행해도 되면 true (저장함 또는 저장 안 함), 취소했으면 false
     */
    bool ConfirmSaveChanges(int index);

    /** @brief 탭 표시줄의 이름과 변경 표시를 문서에 맞춥니다. */
    void UpdateTabTitle(int index);
    /** @brief 창 제목을 "*제목 - MyPaint" 형식으로 맞춥니다. */
    void UpdateWindowTitle();

    // ----- 고스트 트레이싱 -----

    /** @brief 고스트 원본 탭 번호: 고정한 탭이 있으면 그 탭, 없으면 지금 탭의 바로 왼쪽 탭. 없으면 -1 */
    int GhostSourceTab() const;

    /**
     * @brief 고스트 설정을 화면 전체에 반영합니다.
     * 캔버스에 비출 그림, 도구 모음 단추 상태, 원본 탭의 유령 표시, 상태 표시줄 문구를 맞춥니다.
     * 탭이 바뀌거나 열리고 닫힐 때마다 부릅니다.
     */
    void UpdateGhost();

    HICON m_hIcon = nullptr; ///< 창 아이콘
    HACCEL m_hAccel = nullptr; ///< 단축키 표
    UINT m_dpi = USER_DEFAULT_SCREEN_DPI;
    CFont m_font;            ///< 컨트롤 글꼴 (현재 DPI)

    CCanvasWnd m_canvas;
    CTabBar m_tabBar;
    CColorPalette m_palette;

    std::vector<std::unique_ptr<CPaintDocument>> m_documents; ///< 탭 순서와 같은 문서 목록
    int m_activeTab = -1;                                     ///< 선택된 탭 번호
    int m_nextUntitledNumber = 1;                             ///< 다음 새 탭의 "제목 없음" 번호
    static constexpr int kFileButtonCount = 4;          ///< 새 탭, 열기, 저장, 다른 이름으로 저장
    std::array<CButton, kFileButtonCount> m_fileButtons;
    CToolTipCtrl m_toolTip;                             ///< 파일 단추의 단축키 안내
    std::array<CButton, kToolCount> m_toolButtons;
    CStatic m_sizeLabel;
    CComboBox m_sizeCombo;
    CStatic m_fillLabel;
    CComboBox m_fillCombo;
    CButton m_editColorButton;
    static constexpr int kGhostOpacityCount = 3;        ///< 진하기 단계 수 (25%, 50%, 75%)
    CButton m_ghostToggleButton;                        ///< 고스트 보기 (눌림 = 켜짐)
    CButton m_ghostPinButton;                           ///< 원본 고정 (눌림 = 지금 탭이 고정된 원본)
    CStatic m_ghostOpacityLabel;
    std::array<CButton, kGhostOpacityCount> m_ghostOpacityButtons;
    CStatusBarCtrl m_statusBar;

    // 고스트 트레이싱 상태 (모든 탭이 공유)
    bool m_ghostOn = false;                     ///< 고스트를 켰는지
    int m_ghostOpacity = 50;                    ///< 원본이 비치는 진하기(%)
    CPaintDocument* m_ghostPinned = nullptr;    ///< 원본으로 고정한 탭의 문서 (소유하지 않음, 없으면 바로 왼쪽 탭)

    ToolType m_toolBeforePicker = ToolType::Pencil; ///< 스포이트로 색을 집은 뒤 돌아갈 도구

    // 배치 결과 (OnPaint에서 묶음 이름과 구분선을 그릴 때 씀)
    std::array<CRect, GroupCount> m_groupRects;     ///< 묶음별 컨트롤 영역
    int m_captionTop = 0;                           ///< 묶음 이름 위쪽 y
    int m_bandBottom = 0;                           ///< 도구 모음 아래쪽 y
    int m_minClientWidth = 0;                       ///< 도구 모음이 잘리지 않는 최소 너비
};
