/**
 * @file MyPaintDlg.cpp
 * @brief 주 대화 상자 클래스 구현
 */
#include "pch.h"
#include "MyPaint.h"
#include "MyPaintDlg.h"
#include "ImageFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // 배치 값은 모두 96 DPI 기준 px이며 Scale()로 바꿔 씁니다.
    constexpr int kPadding = 8;         ///< 창 가장자리 여백
    constexpr int kBandTop = 6;         ///< 도구 모음 위 여백
    constexpr int kButtonWidth = 62;    ///< 도구 버튼 너비
    constexpr int kButtonHeight = 26;   ///< 도구 버튼, 콤보 줄 높이
    constexpr int kGap = 4;             ///< 컨트롤 사이 간격
    constexpr int kToolColumns = 4;     ///< 도구 버튼 열 수
    constexpr int kGroupSpacing = 12;   ///< 묶음과 구분선 사이 간격
    constexpr int kLabelWidth = 44;     ///< "굵기", "채우기" 글자 너비
    constexpr int kComboWidth = 150;    ///< 콤보 너비
    constexpr int kComboDropHeight = 240; ///< 콤보 목록 높이
    constexpr int kPaletteGap = 2;      ///< 팔레트 칸 사이 간격
    constexpr int kEditButtonWidth = 64; ///< 색 편집 버튼 너비
    constexpr int kCaptionHeight = 18;  ///< 묶음 이름 높이
    constexpr int kMinClientHeight = 420; ///< 최소 창 높이
    constexpr int kInitialClientWidth = 1100;
    constexpr int kInitialClientHeight = 780;

    constexpr int kFileColumns = 2;         ///< 파일 단추 열 수
    constexpr int kFileButtonMinWidth = 56; ///< 파일 단추 최소 너비
    constexpr int kButtonTextPadding = 20;  ///< 단추 글자 양옆 여백의 합
    constexpr int kOpacityButtonMinWidth = 40; ///< 진하기 단추 최소 너비
    constexpr int kToolTipMaxWidth = 360;   ///< 도구 설명 최대 너비 (넘으면 줄을 바꿈)
    constexpr int kStatusCursorRight = 150;     ///< 상태 표시줄: 마우스 위치 칸 오른쪽 끝
    constexpr int kStatusImageSizeRight = 300;  ///< 상태 표시줄: 그림 크기 칸 오른쪽 끝
    constexpr int kStatusGhostWidth = 340;      ///< 상태 표시줄: 고스트 칸 너비

    /** @brief 고스트 진하기 단계(%). 진하기 단추 순서와 같음 */
    constexpr int kGhostOpacities[] = { 25, 50, 75 };

    constexpr int kDefaultImageWidth = 800;   ///< 새 그림 너비
    constexpr int kDefaultImageHeight = 600;  ///< 새 그림 높이

    /** @brief 굵기 콤보의 선택지(px) */
    constexpr int kSizeChoices[] = { 1, 2, 3, 5, 8, 12, 16, 24, 32 };

    /**
     * @brief 파일 묶음의 단추. 단축키와 같은 동작을 합니다.
     * 2열로 놓습니다 (왼쪽 위부터 가로 순서): 왼쪽 열은 그림 가져오기(새 탭, 열기), 오른쪽 열은 저장.
     */
    struct FileButtonInfo
    {
        UINT id;
        const wchar_t* text;
        const wchar_t* tip;  ///< 마우스를 올리면 보이는 설명 (단축키 안내)
    };
    const FileButtonInfo kFileButtons[] = {
        { IDC_FILE_NEW,     L"새 탭",              L"빈 그림으로 새 탭 열기 (Ctrl+T)" },
        { IDC_FILE_SAVE,    L"저장",               L"현재 탭 저장. 처음이면 저장할 곳을 물어봅니다 (Ctrl+S)" },
        { IDC_FILE_OPEN,    L"열기",               L"그림 파일 열기: PNG, BMP, JPEG (Ctrl+O)" },
        { IDC_FILE_SAVE_AS, L"다른 이름으로 저장", L"현재 탭을 다른 이름이나 형식으로 저장 (Ctrl+Shift+S)" },
    };

    /** @brief 파일 경로에서 폴더 부분 (끝의 \ 포함) */
    CString FolderOf(const CString& path)
    {
        return path.Left(static_cast<int>(::PathFindFileName(path) - static_cast<LPCWSTR>(path)));
    }

    /** @brief 상태 표시줄 칸 */
    enum StatusPart
    {
        StatusCursor,
        StatusImageSize,
        StatusGhost,    ///< 고스트 켜짐 여부와 원본 탭
        StatusHint,
        StatusPartCount,
    };
}

BEGIN_MESSAGE_MAP(CMyPaintDlg, CDialog)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(WM_DPICHANGED, &CMyPaintDlg::OnDpiChanged)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_TOOL_FIRST, IDC_TOOL_LAST, &CMyPaintDlg::OnToolButton)
    ON_CBN_SELCHANGE(IDC_SIZE_COMBO, &CMyPaintDlg::OnSizeComboChanged)
    ON_CBN_SELCHANGE(IDC_FILL_COMBO, &CMyPaintDlg::OnFillComboChanged)
    ON_BN_CLICKED(IDC_EDIT_COLOR, &CMyPaintDlg::OnEditColor)
    ON_MESSAGE(WM_CANVAS_CURSOR, &CMyPaintDlg::OnCanvasCursor)
    ON_MESSAGE(WM_CANVAS_COLORPICKED, &CMyPaintDlg::OnCanvasColorPicked)
    ON_MESSAGE(WM_PALETTE_COLORCHANGED, &CMyPaintDlg::OnPaletteColorChanged)
    ON_MESSAGE(WM_CANVAS_MODIFIED, &CMyPaintDlg::OnCanvasModified)
    ON_MESSAGE(WM_TABBAR_ACTION, &CMyPaintDlg::OnTabBarAction)
    ON_COMMAND(ID_TAB_NEW, &CMyPaintDlg::OnNewTab)
    ON_COMMAND(ID_TAB_OPEN, &CMyPaintDlg::OnOpenImage)
    ON_COMMAND(ID_TAB_CLOSE, &CMyPaintDlg::OnCloseTab)
    ON_COMMAND(ID_TAB_SAVE, &CMyPaintDlg::OnSaveTab)
    ON_COMMAND(ID_TAB_SAVE_AS, &CMyPaintDlg::OnSaveTabAs)
    ON_COMMAND(ID_TAB_NEXT, &CMyPaintDlg::OnNextTab)
    ON_COMMAND(ID_TAB_PREV, &CMyPaintDlg::OnPrevTab)
    // 파일 묶음 단추는 단축키와 같은 처리기를 씀
    ON_BN_CLICKED(IDC_FILE_NEW, &CMyPaintDlg::OnNewTab)
    ON_BN_CLICKED(IDC_FILE_OPEN, &CMyPaintDlg::OnOpenImage)
    ON_BN_CLICKED(IDC_FILE_SAVE, &CMyPaintDlg::OnSaveTab)
    ON_BN_CLICKED(IDC_FILE_SAVE_AS, &CMyPaintDlg::OnSaveTabAs)
    // 고스트 트레이싱: 단축키와 단추가 같은 처리기를 씀
    ON_COMMAND(ID_GHOST_TOGGLE, &CMyPaintDlg::OnToggleGhost)
    ON_COMMAND(ID_GHOST_PIN, &CMyPaintDlg::OnPinGhostSource)
    ON_BN_CLICKED(IDC_GHOST_TOGGLE, &CMyPaintDlg::OnToggleGhost)
    ON_BN_CLICKED(IDC_GHOST_PIN, &CMyPaintDlg::OnPinGhostSource)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_GHOST_OPACITY_FIRST, IDC_GHOST_OPACITY_LAST, &CMyPaintDlg::OnGhostOpacity)
END_MESSAGE_MAP()

static_assert(IDC_TOOL_LAST - IDC_TOOL_FIRST + 1 == kToolCount, "도구 버튼 ID 수와 도구 수가 같아야 합니다.");

CMyPaintDlg::CMyPaintDlg(CWnd* pParent) : CDialog(IDD_MYPAINT_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMyPaintDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BOOL CMyPaintDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetIcon(m_hIcon, TRUE);  // 큰 아이콘
    SetIcon(m_hIcon, FALSE); // 작은 아이콘

    m_dpi = ::GetDpiForWindow(GetSafeHwnd());
    m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR));
    CreateControls();
    UpdateUiFont();

    SyncColorsToCanvas();
    SelectTool(ToolType::Pencil);
    if (AddNewTab() < 0)
    {
        EndDialog(IDABORT);
        return TRUE;
    }

    // 처음 크기: 작업 영역 안에 들어가도록 줄이고 가운데에 놓음
    LayoutControls(); // 최소 너비 계산
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    ::GetMonitorInfo(::MonitorFromWindow(GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &mi);
    const CRect work(mi.rcWork);
    CRect rc(0, 0, max(m_minClientWidth, Scale(kInitialClientWidth)), Scale(kInitialClientHeight));
    ::AdjustWindowRectExForDpi(&rc, GetStyle(), FALSE, GetExStyle(), m_dpi);
    const int width = min(rc.Width(), work.Width());
    const int height = min(rc.Height(), work.Height());
    SetWindowPos(nullptr, work.left + (work.Width() - width) / 2, work.top + (work.Height() - height) / 2,
                 width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    LayoutControls();

    return TRUE;
}

int CMyPaintDlg::Scale(int value) const
{
    return ::MulDiv(value, static_cast<int>(m_dpi), USER_DEFAULT_SCREEN_DPI);
}

int CMyPaintDlg::ButtonTextWidth(const CString& text)
{
    CClientDC dc(this);
    CFont* oldFont = (m_font.GetSafeHandle() != nullptr) ? dc.SelectObject(&m_font) : nullptr;
    const int width = dc.GetTextExtent(text).cx + Scale(kButtonTextPadding);
    if (oldFont != nullptr)
    {
        dc.SelectObject(oldFont);
    }
    return width;
}

void CMyPaintDlg::UpdateUiFont()
{
    NONCLIENTMETRICS ncm{};
    ncm.cbSize = sizeof(ncm);
    ::SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, m_dpi);

    CFont font;
    if (!font.CreateFontIndirect(&ncm.lfMessageFont))
    {
        return;
    }
    // 새 글꼴을 먼저 적용한 뒤 이전 글꼴을 지움 (컨트롤이 지워진 글꼴을 쓰지 않도록)
    SendMessageToDescendants(WM_SETFONT, reinterpret_cast<WPARAM>(font.GetSafeHandle()), TRUE, FALSE);
    m_font.DeleteObject();
    m_font.Attach(font.Detach());
}

void CMyPaintDlg::CreateControls()
{
    const CRect empty(0, 0, 0, 0);

    // 파일 단추: 새 탭 / 열기 / 저장 / 다른 이름으로 저장 (맨 왼쪽, 그림판의 파일 메뉴 자리)
    static_assert(_countof(kFileButtons) == kFileButtonCount, "파일 단추 표와 단추 수가 같아야 합니다.");
    for (int i = 0; i < kFileButtonCount; ++i)
    {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
        if (i == 0)
        {
            style |= WS_GROUP;
        }
        m_fileButtons[i].Create(kFileButtons[i].text, style, empty, this, kFileButtons[i].id);
    }

    // 도구 버튼: 누른 상태가 유지되는 라디오 버튼 묶음
    for (int i = 0; i < kToolCount; ++i)
    {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | BS_PUSHLIKE;
        if (i == 0)
        {
            style |= WS_GROUP;
        }
        m_toolButtons[i].Create(GetToolInfo(static_cast<ToolType>(i)).name, style, empty, this, IDC_TOOL_FIRST + i);
    }

    m_sizeLabel.Create(L"굵기", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, empty, this);
    m_sizeCombo.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, empty, this, IDC_SIZE_COMBO);
    for (int size : kSizeChoices)
    {
        CString text;
        text.Format(L"%dpx", size);
        const int index = m_sizeCombo.AddString(text);
        m_sizeCombo.SetItemData(index, static_cast<DWORD_PTR>(size));
    }

    m_fillLabel.Create(L"채우기", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, empty, this);
    m_fillCombo.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, empty, this, IDC_FILL_COMBO);
    // 항목 순서는 ShapeFillMode 값과 같음
    m_fillCombo.AddString(L"윤곽선만");
    m_fillCombo.AddString(L"윤곽선 + 색 2로 채우기");
    m_fillCombo.AddString(L"채우기만");
    m_fillCombo.SetCurSel(static_cast<int>(m_canvas.GetFillMode()));

    m_palette.Create(empty, this, IDC_PALETTE);
    m_editColorButton.Create(L"색 편집", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_PUSHBUTTON | BS_MULTILINE,
                             empty, this, IDC_EDIT_COLOR);

    // 고스트 트레이싱: 보기(켜기/끄기)와 원본 고정은 눌린 모양이 상태를 보여 주는 단추.
    // 눌림 상태는 UpdateGhost가 정하므로 저절로 바뀌지 않는 BS_CHECKBOX를 씀
    m_ghostToggleButton.Create(L"고스트 보기", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_CHECKBOX | BS_PUSHLIKE,
                               empty, this, IDC_GHOST_TOGGLE);
    m_ghostPinButton.Create(L"원본 고정", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                            empty, this, IDC_GHOST_PIN);
    m_ghostOpacityLabel.Create(L"진하기", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, empty, this);
    static_assert(_countof(kGhostOpacities) == kGhostOpacityCount, "진하기 표와 단추 수가 같아야 합니다.");
    static_assert(IDC_GHOST_OPACITY_LAST - IDC_GHOST_OPACITY_FIRST + 1 == kGhostOpacityCount,
                  "진하기 단추 ID 수와 단추 수가 같아야 합니다.");
    for (int i = 0; i < kGhostOpacityCount; ++i)
    {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | BS_PUSHLIKE;
        if (i == 0)
        {
            style |= WS_GROUP;
        }
        CString text;
        text.Format(L"%d%%", kGhostOpacities[i]);
        m_ghostOpacityButtons[i].Create(text, style, empty, this, IDC_GHOST_OPACITY_FIRST + i);
        if (kGhostOpacities[i] == m_ghostOpacity)
        {
            m_ghostOpacityButtons[i].SetCheck(BST_CHECKED);
        }
    }

    m_tabBar.Create(empty, this, IDC_TABBAR);
    m_tabBar.SetActiveColor(kCanvasWorkspaceColor);
    m_canvas.Create(empty, this, IDC_CANVAS);
    m_statusBar.Create(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM, empty, this, IDC_STATUSBAR);
    UpdateStatusBarParts(); // 칸을 먼저 나눠야 배치 전에 넣는 글자(도구 설명 등)가 사라지지 않음

    // 단추에 마우스를 올리면 하는 일과 단축키를 알려 줌
    if (m_toolTip.Create(this, TTS_ALWAYSTIP))
    {
        m_toolTip.SetMaxTipWidth(Scale(kToolTipMaxWidth));
        for (int i = 0; i < kFileButtonCount; ++i)
        {
            m_toolTip.AddTool(&m_fileButtons[i], kFileButtons[i].tip);
        }
        m_toolTip.AddTool(&m_ghostToggleButton,
                          L"다른 탭의 그림을 지금 탭에 반투명하게 비춰, 그 위에 대고 따라 그립니다 (Ctrl+G). "
                          L"화면에만 보이고 저장되지 않습니다.");
        m_toolTip.AddTool(&m_ghostPinButton,
                          L"지금 탭을 고스트 원본으로 고정하거나 풉니다 (Ctrl+Shift+G). "
                          L"고정하지 않으면 바로 왼쪽 탭을 비춥니다.");
        for (CButton& button : m_ghostOpacityButtons)
        {
            m_toolTip.AddTool(&button, L"고스트 원본이 비치는 진하기");
        }
        m_toolTip.Activate(TRUE);
    }
}

void CMyPaintDlg::LayoutControls()
{
    if (m_canvas.GetSafeHwnd() == nullptr)
    {
        return;
    }

    CRect rcClient;
    GetClientRect(&rcClient);

    const int top = Scale(kBandTop);
    const int buttonW = Scale(kButtonWidth);
    const int buttonH = Scale(kButtonHeight);
    const int gap = Scale(kGap);
    const int bandH = 2 * buttonH + gap; // 컨트롤 두 줄
    const int spacing = Scale(kGroupSpacing);
    int x = Scale(kPadding);

    // 파일: 2×2 단추. 열 너비는 그 열에서 가장 긴 글자에 맞춤 ("다른 이름으로 저장")
    {
        int columnWidth[kFileColumns];
        for (int& width : columnWidth)
        {
            width = Scale(kFileButtonMinWidth);
        }
        for (int i = 0; i < kFileButtonCount; ++i)
        {
            columnWidth[i % kFileColumns] = max(columnWidth[i % kFileColumns], ButtonTextWidth(kFileButtons[i].text));
        }

        for (int i = 0; i < kFileButtonCount; ++i)
        {
            const int col = i % kFileColumns;
            const int row = i / kFileColumns;
            const int buttonX = x + (col == 0 ? 0 : columnWidth[0] + gap);
            m_fileButtons[i].MoveWindow(buttonX, top + row * (buttonH + gap), columnWidth[col], buttonH);
        }
        m_groupRects[GroupFile].SetRect(x, top, x + columnWidth[0] + gap + columnWidth[1], top + bandH);
        x = m_groupRects[GroupFile].right + 2 * spacing;
    }

    // 도구
    for (int i = 0; i < kToolCount; ++i)
    {
        const int col = i % kToolColumns;
        const int row = i / kToolColumns;
        m_toolButtons[i].MoveWindow(x + col * (buttonW + gap), top + row * (buttonH + gap), buttonW, buttonH);
    }
    const int toolRows = (kToolCount + kToolColumns - 1) / kToolColumns;
    m_groupRects[GroupTools].SetRect(x, top, x + kToolColumns * buttonW + (kToolColumns - 1) * gap,
                                     top + toolRows * buttonH + (toolRows - 1) * gap);
    x = m_groupRects[GroupTools].right + 2 * spacing;

    // 모양: 굵기 / 채우기
    const int labelW = Scale(kLabelWidth);
    const int comboW = Scale(kComboWidth);
    auto placeRow = [&](CStatic& label, CComboBox& combo, int rowTop) {
        label.MoveWindow(x, rowTop, labelW, buttonH);
        // 드롭다운 콤보의 높이는 목록 높이이고, 닫힌 높이는 글꼴로 정해지므로 만든 뒤 가운데로 맞춤
        combo.MoveWindow(x + labelW, rowTop, comboW, Scale(kComboDropHeight));
        CRect rcCombo;
        combo.GetWindowRect(&rcCombo);
        combo.MoveWindow(x + labelW, rowTop + (buttonH - rcCombo.Height()) / 2, comboW, Scale(kComboDropHeight));
    };
    placeRow(m_sizeLabel, m_sizeCombo, top);
    placeRow(m_fillLabel, m_fillCombo, top + buttonH + gap);
    m_groupRects[GroupShape].SetRect(x, top, x + labelW + comboW, top + bandH);
    x = m_groupRects[GroupShape].right + 2 * spacing;

    // 색: 팔레트 + 색 편집 버튼
    const int paletteGap = Scale(kPaletteGap);
    const int paletteH = bandH - 2 * gap;
    const int paletteW = CColorPalette::CalcWidth(paletteH, paletteGap);
    m_palette.MoveWindow(x, top + (bandH - paletteH) / 2, paletteW, paletteH);
    const int editX = x + paletteW + spacing;
    m_editColorButton.MoveWindow(editX, top, Scale(kEditButtonWidth), bandH);
    m_groupRects[GroupColors].SetRect(x, top, editX + Scale(kEditButtonWidth), top + bandH);
    x = m_groupRects[GroupColors].right + 2 * spacing;

    // 고스트 트레이싱: [고스트 보기][원본 고정] / 진하기 [25%][50%][75%]
    {
        auto textWidthOf = [this](CWnd& wnd) {
            CString text;
            wnd.GetWindowText(text);
            return ButtonTextWidth(text);
        };
        const int toggleW = max(textWidthOf(m_ghostToggleButton), textWidthOf(m_ghostPinButton));
        int opacityW = Scale(kOpacityButtonMinWidth);
        for (CButton& button : m_ghostOpacityButtons)
        {
            opacityW = max(opacityW, textWidthOf(button));
        }
        const int groupW = max(2 * toggleW + gap, labelW + kGhostOpacityCount * opacityW + (kGhostOpacityCount - 1) * gap);

        // 윗줄: 두 단추가 묶음 너비를 반씩 나눔
        const int halfW = (groupW - gap) / 2;
        m_ghostToggleButton.MoveWindow(x, top, halfW, buttonH);
        m_ghostPinButton.MoveWindow(x + groupW - halfW, top, halfW, buttonH);

        // 아랫줄: 진하기 글자 + 단계 단추
        const int rowTop = top + buttonH + gap;
        m_ghostOpacityLabel.MoveWindow(x, rowTop, labelW, buttonH);
        const int stepW = (groupW - labelW - (kGhostOpacityCount - 1) * gap) / kGhostOpacityCount;
        for (int i = 0; i < kGhostOpacityCount; ++i)
        {
            m_ghostOpacityButtons[i].MoveWindow(x + labelW + i * (stepW + gap), rowTop, stepW, buttonH);
        }
        m_groupRects[GroupGhost].SetRect(x, top, x + groupW, top + bandH);
    }

    m_minClientWidth = m_groupRects[GroupGhost].right + Scale(kPadding);
    m_captionTop = top + bandH + gap / 2;
    m_bandBottom = m_captionTop + Scale(kCaptionHeight) + gap / 2;

    // 상태 표시줄 (CCS_BOTTOM이라 WM_SIZE를 받으면 스스로 아래에 붙음)
    m_statusBar.SendMessage(WM_SIZE);
    CRect rcStatus;
    m_statusBar.GetWindowRect(&rcStatus);
    UpdateStatusBarParts(); // DPI가 바뀌었을 수 있음

    // 탭 표시줄: 도구 모음 바로 아래. 선택된 탭이 아래 캔버스와 이어짐
    const int tabTop = m_bandBottom + 1;
    const int tabH = m_tabBar.GetPreferredHeight();
    m_tabBar.MoveWindow(0, tabTop, rcClient.Width(), tabH);

    // 캔버스: 탭 표시줄과 상태 표시줄 사이 전체
    const int canvasTop = tabTop + tabH;
    m_canvas.MoveWindow(0, canvasTop, rcClient.Width(), max(0, rcClient.Height() - rcStatus.Height() - canvasTop));

    InvalidateRect(CRect(0, 0, rcClient.Width(), m_bandBottom + 1));
}

void CMyPaintDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    // 묶음 이름
    CFont* oldFont = dc.SelectObject(&m_font);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    static const wchar_t* const kCaptions[GroupCount] = { L"파일", L"도구", L"모양", L"색", L"고스트 트레이싱" };
    for (int i = 0; i < GroupCount; ++i)
    {
        CRect rcCaption(m_groupRects[i].left, m_captionTop, m_groupRects[i].right, m_captionTop + Scale(kCaptionHeight));
        dc.DrawText(kCaptions[i], -1, &rcCaption, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    dc.SelectObject(oldFont);

    // 묶음 사이 세로 구분선, 도구 모음 아래 가로 구분선
    const COLORREF lineColor = ::GetSysColor(COLOR_3DSHADOW);
    const int spacing = Scale(kGroupSpacing);
    for (int i = 0; i + 1 < GroupCount; ++i)
    {
        const int lineX = m_groupRects[i].right + spacing;
        dc.FillSolidRect(lineX, m_groupRects[i].top, 1, m_bandBottom - m_groupRects[i].top - Scale(kGap), lineColor);
    }
    dc.FillSolidRect(0, m_bandBottom, rcClient.Width(), 1, lineColor);
}

void CMyPaintDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (nType != SIZE_MINIMIZED)
    {
        LayoutControls();
    }
}

void CMyPaintDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CDialog::OnGetMinMaxInfo(lpMMI);
    if (m_minClientWidth > 0)
    {
        CRect rc(0, 0, m_minClientWidth, Scale(kMinClientHeight));
        ::AdjustWindowRectExForDpi(&rc, GetStyle(), FALSE, GetExStyle(), m_dpi);
        lpMMI->ptMinTrackSize.x = rc.Width();
        lpMMI->ptMinTrackSize.y = rc.Height();
    }
}

LRESULT CMyPaintDlg::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    m_dpi = LOWORD(wParam);
    UpdateUiFont();

    const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
    SetWindowPos(nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                 suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
    LayoutControls();
    Invalidate();
    return 0;
}

void CMyPaintDlg::OnOK()
{
    // Enter 키로 닫지 않음
}

void CMyPaintDlg::OnCancel()
{
    // Esc 키: 진행 중인 그리기만 취소하고 닫지 않음
    m_canvas.CancelStroke();
}

void CMyPaintDlg::OnClose()
{
    // 저장하지 않은 탭마다 물어보고, 하나라도 취소하면 닫지 않음
    m_canvas.CancelStroke();
    for (int i = 0; i < static_cast<int>(m_documents.size()); ++i)
    {
        if (!ConfirmSaveChanges(i))
        {
            return;
        }
    }
    EndDialog(IDCANCEL);
}

BOOL CMyPaintDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd() != nullptr)
    {
        m_toolTip.RelayEvent(pMsg); // 도구 설명이 마우스 움직임을 알 수 있게
    }
    if (m_hAccel != nullptr && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST &&
        ::TranslateAccelerator(GetSafeHwnd(), m_hAccel, pMsg))
    {
        return TRUE;
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// ---------------------------------------------------------------------------
// 탭
// ---------------------------------------------------------------------------

int CMyPaintDlg::AddNewTab()
{
    auto document = std::make_unique<CPaintDocument>(m_nextUntitledNumber);
    if (!document->Create(kDefaultImageWidth, kDefaultImageHeight))
    {
        MessageBox(L"새 그림을 만들지 못했습니다. 메모리가 부족할 수 있습니다.", L"MyPaint", MB_ICONERROR);
        return -1;
    }
    ++m_nextUntitledNumber;

    const int index = AppendDocument(std::move(document));
    ActivateTab(index);
    return index;
}

int CMyPaintDlg::AppendDocument(std::unique_ptr<CPaintDocument> document)
{
    const int index = static_cast<int>(m_documents.size()); // 크롬처럼 맨 끝에 추가
    m_tabBar.InsertTab(index, document->GetTitle());
    m_documents.push_back(std::move(document));
    return index;
}

void CMyPaintDlg::ReplaceDocument(int index, std::unique_ptr<CPaintDocument> document)
{
    const bool active = (index == m_activeTab);
    if (active)
    {
        m_canvas.SetDocument(nullptr); // 지울 문서를 캔버스가 더 가리키지 않게
    }
    m_canvas.SetGhost(nullptr, m_ghostOpacity); // 지울 문서를 고스트로 비추고 있었을 수도 있음
    if (m_documents[index].get() == m_ghostPinned)
    {
        m_ghostPinned = document.get(); // 원본 고정은 문서가 아니라 탭을 따라감
    }
    m_documents[index] = std::move(document);
    UpdateTabTitle(index);
    if (active)
    {
        ActivateTab(index);
    }
    UpdateGhost();
}

int CMyPaintDlg::FindTabByPath(const CString& path) const
{
    for (int i = 0; i < static_cast<int>(m_documents.size()); ++i)
    {
        const CString& tabPath = m_documents[i]->GetFilePath();
        if (!tabPath.IsEmpty() && tabPath.CompareNoCase(path) == 0)
        {
            return i;
        }
    }
    return -1;
}

void CMyPaintDlg::OpenImageFiles(const std::vector<CString>& paths)
{
    // 지금 탭이 아무것도 안 그린 빈 탭이면 첫 파일은 그 자리에 엶 (빈 탭이 남지 않게)
    int reusableTab = (m_activeTab >= 0 && m_documents[m_activeTab]->IsPristine()) ? m_activeTab : -1;
    int tabToShow = -1;
    CString failures;
    {
        CWaitCursor wait;
        for (const CString& path : paths)
        {
            const int existing = FindTabByPath(path);
            if (existing >= 0)
            {
                tabToShow = existing; // 이미 열린 파일은 다시 열지 않고 그 탭으로 감
                continue;
            }

            auto document = std::make_unique<CPaintDocument>(0);
            CString error;
            if (!document->LoadFromFile(path, error))
            {
                failures.AppendFormat(L"\n\n%s\n%s", static_cast<LPCWSTR>(::PathFindFileName(path)), static_cast<LPCWSTR>(error));
                continue;
            }
            if (reusableTab >= 0)
            {
                ReplaceDocument(reusableTab, std::move(document));
                tabToShow = reusableTab;
                reusableTab = -1;
            }
            else
            {
                tabToShow = AppendDocument(std::move(document));
            }
        }
    }

    if (tabToShow >= 0)
    {
        ActivateTab(tabToShow);
    }
    if (!failures.IsEmpty())
    {
        MessageBox(L"다음 파일을 열지 못했습니다." + failures, L"열기 실패", MB_ICONERROR);
    }
}

bool CMyPaintDlg::AskOpenPaths(std::vector<CString>& paths)
{
    static const wchar_t kFilter[] =
        L"그림 파일 (*.png;*.bmp;*.jpg;*.jpeg)|*.png;*.bmp;*.jpg;*.jpeg|"
        L"PNG (*.png)|*.png|BMP (*.bmp)|*.bmp|JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg|모든 파일 (*.*)|*.*||";

    CFileDialog dlg(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
                    kFilter, this);
    // 여러 파일을 고르면 이름이 이어 붙으므로 넉넉한 버퍼를 줌
    std::vector<wchar_t> buffer(64 * 1024, L'\0');
    dlg.m_ofn.lpstrFile = buffer.data();
    dlg.m_ofn.nMaxFile = static_cast<DWORD>(buffer.size());

    // 지금 탭이 파일이면 그 폴더에서 시작
    CString folder;
    if (m_activeTab >= 0 && !m_documents[m_activeTab]->GetFilePath().IsEmpty())
    {
        folder = FolderOf(m_documents[m_activeTab]->GetFilePath());
        dlg.m_ofn.lpstrInitialDir = folder;
    }

    if (dlg.DoModal() != IDOK)
    {
        return false;
    }
    POSITION pos = dlg.GetStartPosition();
    while (pos != nullptr)
    {
        paths.push_back(dlg.GetNextPathName(pos));
    }
    return !paths.empty();
}

void CMyPaintDlg::ActivateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_documents.size()))
    {
        return;
    }
    m_activeTab = index;
    m_canvas.SetDocument(m_documents[index].get()); // 도구 설정은 캔버스에 그대로 남음
    m_tabBar.SetActiveTab(index);
    UpdateWindowTitle();
    UpdateImageSizeStatus();
    m_statusBar.SetText(L"", StatusCursor, 0);
    UpdateGhost(); // 바로 왼쪽 탭이 바뀌고, 원본 탭 자신이면 겹치지 않음
}

bool CMyPaintDlg::CloseTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_documents.size()))
    {
        return false;
    }
    if (!ConfirmSaveChanges(index)) // 저장할지 물을 때 그 탭을 보여 주므로 선택이 바뀔 수 있음
    {
        return false;
    }

    const bool wasActive = (index == m_activeTab);
    if (wasActive)
    {
        m_canvas.SetDocument(nullptr); // 지울 문서를 캔버스가 더 가리키지 않게
    }
    m_canvas.SetGhost(nullptr, m_ghostOpacity); // 지울 문서를 고스트로 비추고 있었을 수도 있음
    if (m_documents[index].get() == m_ghostPinned)
    {
        m_ghostPinned = nullptr; // 고정한 원본 탭을 닫으면 다시 바로 왼쪽 탭을 비춤
    }
    m_documents.erase(m_documents.begin() + index);
    m_tabBar.RemoveTab(index);

    if (m_documents.empty())
    {
        m_activeTab = -1;
        AddNewTab(); // 창은 항상 탭을 하나 이상 가짐
    }
    else if (wasActive)
    {
        // 크롬처럼 오른쪽 탭으로, 마지막 탭이었다면 왼쪽 탭으로
        ActivateTab(min(index, static_cast<int>(m_documents.size()) - 1));
    }
    else
    {
        if (index < m_activeTab)
        {
            --m_activeTab;
        }
        m_tabBar.SetActiveTab(m_activeTab);
    }
    UpdateGhost(); // 다른 탭을 닫아도 바로 왼쪽 탭이나 고정 원본이 바뀔 수 있음
    return true;
}

bool CMyPaintDlg::ConfirmSaveChanges(int index)
{
    CPaintDocument& document = *m_documents[index];
    if (!document.IsModified())
    {
        return true;
    }
    if (index != m_activeTab)
    {
        ActivateTab(index); // 무엇을 저장하는지 보이게
    }

    CString message;
    message.Format(L"'%s'의 변경 내용을 저장할까요?", static_cast<LPCWSTR>(document.GetTitle()));
    switch (MessageBox(message, L"MyPaint", MB_YESNOCANCEL | MB_ICONWARNING))
    {
    case IDYES: return SaveTab(index, false); // 저장을 취소하거나 실패하면 닫지 않음
    case IDNO:  return true;
    default:    return false;
    }
}

bool CMyPaintDlg::SaveTab(int index, bool askPath)
{
    CPaintDocument& document = *m_documents[index];

    // 처음 저장하는 탭이면 경로를 묻고, 이미 저장한 탭이면 묻지 않고 덮어씀.
    // 저장할 수 없는 형식으로 불러온 파일(GIF 등)도 저장할 곳을 물어봄
    CString path = document.GetFilePath();
    ImageFormat format;
    if (path.IsEmpty() || askPath || !ImageFormatFromPath(path, format))
    {
        if (!AskSavePath(document, path))
        {
            return false;
        }
    }

    CString error;
    {
        CWaitCursor wait;
        if (!SaveImageFile(document.Surface(), path, error))
        {
            MessageBox(error, L"저장 실패", MB_ICONERROR);
            return false;
        }
    }

    document.SetFilePath(path);
    document.SetModified(false);
    UpdateTabTitle(index);
    if (index == m_activeTab)
    {
        UpdateWindowTitle();
    }
    UpdateGhost(); // 상태 표시줄의 원본 이름이 바뀌었을 수 있음
    return true;
}

bool CMyPaintDlg::AskSavePath(const CPaintDocument& document, CString& path)
{
    // 필터 순서와 kExtensions 순서가 같아야 함 (nFilterIndex는 1부터)
    static const wchar_t kFilter[] = L"PNG (*.png)|*.png|BMP (*.bmp)|*.bmp|JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg||";
    static const wchar_t* const kExtensions[] = { L".png", L".bmp", L".jpg" };

    const CString currentPath = document.GetFilePath();
    CString fileName;
    CString folder;
    DWORD filterIndex = 1; // PNG 기본
    if (currentPath.IsEmpty())
    {
        fileName = document.GetTitle() + L".png";
    }
    else
    {
        fileName = ::PathFindFileName(currentPath);
        folder = FolderOf(currentPath);
        ImageFormat format;
        if (ImageFormatFromPath(currentPath, format))
        {
            filterIndex = static_cast<DWORD>(format) + 1;
        }
        else
        {
            // 저장할 수 없는 형식(GIF 등)으로 불러온 그림은 같은 이름의 PNG를 권함
            fileName = fileName.Left(static_cast<int>(::PathFindExtension(fileName) - static_cast<LPCWSTR>(fileName))) + L".png";
        }
    }

    CFileDialog dlg(FALSE, L"png", fileName, OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, kFilter, this);
    dlg.m_ofn.nFilterIndex = filterIndex;
    if (!folder.IsEmpty())
    {
        dlg.m_ofn.lpstrInitialDir = folder;
    }
    if (dlg.DoModal() != IDOK)
    {
        return false;
    }

    CString chosen = dlg.GetPathName();
    ImageFormat format;
    if (!ImageFormatFromPath(chosen, format))
    {
        // 확장자가 없거나 지원하지 않으면 고른 형식의 확장자를 붙임.
        // 이 경우 대화 상자의 덮어쓰기 확인을 거치지 않았으므로 직접 확인
        const DWORD selected = dlg.m_ofn.nFilterIndex;
        chosen += kExtensions[(selected >= 1 && selected <= _countof(kExtensions)) ? selected - 1 : 0];
        if (::PathFileExists(chosen))
        {
            CString message;
            message.Format(L"%s이(가) 이미 있습니다.\n바꾸시겠습니까?", static_cast<LPCWSTR>(::PathFindFileName(chosen)));
            if (MessageBox(message, L"다른 이름으로 저장 확인", MB_YESNO | MB_ICONWARNING) != IDYES)
            {
                return false;
            }
        }
    }
    path = chosen;
    return true;
}

void CMyPaintDlg::UpdateTabTitle(int index)
{
    const CPaintDocument& document = *m_documents[index];
    m_tabBar.SetTab(index, document.GetTitle(), document.IsModified());
}

void CMyPaintDlg::UpdateWindowTitle()
{
    CString title(L"MyPaint");
    if (m_activeTab >= 0)
    {
        const CPaintDocument& document = *m_documents[m_activeTab];
        title.Format(L"%s%s - MyPaint", document.IsModified() ? L"*" : L"", static_cast<LPCWSTR>(document.GetTitle()));
    }
    SetWindowText(title);
}

void CMyPaintDlg::OnNewTab()
{
    AddNewTab();
}

void CMyPaintDlg::OnOpenImage()
{
    if (m_canvas.IsStroking()) // 그리는 도중(마우스를 누른 채 Ctrl+O)에는 열지 않음
    {
        return;
    }
    std::vector<CString> paths;
    if (AskOpenPaths(paths))
    {
        OpenImageFiles(paths);
    }
}

void CMyPaintDlg::OnCloseTab()
{
    CloseTab(m_activeTab);
}

void CMyPaintDlg::OnSaveTab()
{
    if (m_activeTab >= 0 && !m_canvas.IsStroking()) // 그리는 도중의 반쯤 그린 상태는 저장하지 않음
    {
        SaveTab(m_activeTab, false);
    }
}

void CMyPaintDlg::OnSaveTabAs()
{
    if (m_activeTab >= 0 && !m_canvas.IsStroking())
    {
        SaveTab(m_activeTab, true);
    }
}

void CMyPaintDlg::OnNextTab()
{
    const int count = static_cast<int>(m_documents.size());
    if (count > 1)
    {
        ActivateTab((m_activeTab + 1) % count);
    }
}

void CMyPaintDlg::OnPrevTab()
{
    const int count = static_cast<int>(m_documents.size());
    if (count > 1)
    {
        ActivateTab((m_activeTab + count - 1) % count);
    }
}

LRESULT CMyPaintDlg::OnTabBarAction(WPARAM wParam, LPARAM lParam)
{
    const int index = static_cast<int>(lParam);
    switch (static_cast<TabBarAction>(wParam))
    {
    case TabBarAction::Select:
        if (index != m_activeTab)
        {
            ActivateTab(index);
        }
        break;
    case TabBarAction::Close:
        CloseTab(index);
        break;
    case TabBarAction::NewTab:
        AddNewTab();
        break;
    }
    return 0;
}

LRESULT CMyPaintDlg::OnCanvasModified(WPARAM, LPARAM)
{
    UpdateTabTitle(m_activeTab);
    UpdateWindowTitle();
    return 0;
}

// ---------------------------------------------------------------------------
// 고스트 트레이싱
// ---------------------------------------------------------------------------

int CMyPaintDlg::GhostSourceTab() const
{
    if (m_ghostPinned != nullptr)
    {
        for (int i = 0; i < static_cast<int>(m_documents.size()); ++i)
        {
            if (m_documents[i].get() == m_ghostPinned)
            {
                return i;
            }
        }
    }
    // 고정한 원본이 없으면 바로 왼쪽(이전) 탭. 맨 왼쪽 탭에는 없음
    return (m_activeTab > 0 && m_activeTab < static_cast<int>(m_documents.size())) ? m_activeTab - 1 : -1;
}

void CMyPaintDlg::UpdateGhost()
{
    const int source = GhostSourceTab();
    const bool pinned = (source >= 0 && m_documents[source].get() == m_ghostPinned);
    const bool overlay = m_ghostOn && source >= 0 && source != m_activeTab; // 원본 탭 자신을 볼 때는 겹치지 않음

    m_canvas.SetGhost(overlay ? &m_documents[source]->Surface() : nullptr, m_ghostOpacity);

    // 원본 탭 이름 앞의 유령 표시: 켜져 있으면 보라색(고정이면 꽉 참), 꺼져 있으면 고정 원본만 회색으로
    CTabBar::GhostMark mark = CTabBar::GhostMark::None;
    if (m_ghostOn && source >= 0)
    {
        mark = pinned ? CTabBar::GhostMark::PinnedSource : CTabBar::GhostMark::Source;
    }
    else if (pinned)
    {
        mark = CTabBar::GhostMark::PinnedIdle;
    }
    m_tabBar.SetGhostMark(source, mark);

    // 도구 모음 단추: 고스트 보기는 켜져 있을 때, 원본 고정은 지금 탭이 고정 원본일 때 눌림
    m_ghostToggleButton.SetCheck(m_ghostOn ? BST_CHECKED : BST_UNCHECKED);
    m_ghostPinButton.SetCheck((pinned && source == m_activeTab) ? BST_CHECKED : BST_UNCHECKED);

    // 상태 표시줄: 켜져 있으면 진하기와 원본 탭, 꺼져 있으면 고정한 원본만 (없으면 비워 둠)
    CString text;
    const CString sourceTitle = (source >= 0) ? m_documents[source]->GetTitle() : CString();
    if (m_ghostOn)
    {
        if (source < 0)
        {
            text = L"고스트 켜짐 · 비출 왼쪽 탭이 없음";
        }
        else if (source == m_activeTab)
        {
            text = L"고스트 켜짐 · 지금 탭이 원본이라 겹치지 않음";
        }
        else
        {
            text.Format(L"고스트 %d%% · 원본: %s %s", m_ghostOpacity, static_cast<LPCWSTR>(sourceTitle),
                        pinned ? L"(고정)" : L"(왼쪽 탭)");
        }
    }
    else if (pinned)
    {
        text.Format(L"고스트 꺼짐 · 고정한 원본: %s", static_cast<LPCWSTR>(sourceTitle));
    }
    m_statusBar.SetText(text, StatusGhost, 0);
}

void CMyPaintDlg::OnToggleGhost()
{
    m_ghostOn = !m_ghostOn;
    UpdateGhost();
}

void CMyPaintDlg::OnPinGhostSource()
{
    if (m_activeTab < 0)
    {
        return;
    }
    CPaintDocument* current = m_documents[m_activeTab].get();
    m_ghostPinned = (m_ghostPinned == current) ? nullptr : current; // 이미 고정한 탭이면 고정을 풂
    UpdateGhost();
}

void CMyPaintDlg::OnGhostOpacity(UINT nID)
{
    m_ghostOpacity = kGhostOpacities[nID - IDC_GHOST_OPACITY_FIRST];
    UpdateGhost();
}

// ---------------------------------------------------------------------------
// 도구와 옵션
// ---------------------------------------------------------------------------

void CMyPaintDlg::SelectTool(ToolType tool)
{
    if (tool == ToolType::ColorPicker && m_canvas.GetTool() != ToolType::ColorPicker)
    {
        m_toolBeforePicker = m_canvas.GetTool();
    }
    m_canvas.SetTool(tool);
    CheckRadioButton(IDC_TOOL_FIRST, IDC_TOOL_LAST, IDC_TOOL_FIRST + ToIndex(tool));
    UpdateToolOptions();
}

void CMyPaintDlg::UpdateToolOptions()
{
    const ToolType tool = m_canvas.GetTool();
    const ToolInfo& info = GetToolInfo(tool);

    m_sizeCombo.EnableWindow(info.usesSize);
    m_sizeLabel.EnableWindow(info.usesSize);
    const int size = m_canvas.GetToolSize(tool);
    int selection = -1;
    for (int i = 0; i < m_sizeCombo.GetCount(); ++i)
    {
        if (static_cast<int>(m_sizeCombo.GetItemData(i)) == size)
        {
            selection = i;
            break;
        }
    }
    m_sizeCombo.SetCurSel(info.usesSize ? selection : -1);

    m_fillCombo.EnableWindow(info.usesFillMode);
    m_fillLabel.EnableWindow(info.usesFillMode);

    m_statusBar.SetText(info.hint, StatusHint, 0);
}

void CMyPaintDlg::OnToolButton(UINT nID)
{
    SelectTool(static_cast<ToolType>(nID - IDC_TOOL_FIRST));
}

void CMyPaintDlg::OnSizeComboChanged()
{
    const int selection = m_sizeCombo.GetCurSel();
    if (selection != CB_ERR)
    {
        m_canvas.SetToolSize(m_canvas.GetTool(), static_cast<int>(m_sizeCombo.GetItemData(selection)));
    }
}

void CMyPaintDlg::OnFillComboChanged()
{
    const int selection = m_fillCombo.GetCurSel();
    if (selection != CB_ERR)
    {
        m_canvas.SetFillMode(static_cast<ShapeFillMode>(selection));
    }
}

// ---------------------------------------------------------------------------
// 색
// ---------------------------------------------------------------------------

void CMyPaintDlg::SyncColorsToCanvas()
{
    m_canvas.SetColors(m_palette.GetPrimary(), m_palette.GetSecondary());
}

void CMyPaintDlg::OnEditColor()
{
    m_palette.EditPrimary(); // 바뀌면 팔레트가 WM_PALETTE_COLORCHANGED를 보냄
}

LRESULT CMyPaintDlg::OnPaletteColorChanged(WPARAM, LPARAM)
{
    SyncColorsToCanvas();
    return 0;
}

LRESULT CMyPaintDlg::OnCanvasColorPicked(WPARAM wParam, LPARAM lParam)
{
    const COLORREF color = static_cast<COLORREF>(lParam);
    if (static_cast<MouseButton>(wParam) == MouseButton::Left)
    {
        m_palette.SetPrimary(color);
    }
    else
    {
        m_palette.SetSecondary(color);
    }
    SyncColorsToCanvas();
    SelectTool(m_toolBeforePicker); // 그림판처럼 색을 집은 뒤 이전 도구로 돌아감
    return 0;
}

// ---------------------------------------------------------------------------
// 상태 표시줄
// ---------------------------------------------------------------------------

LRESULT CMyPaintDlg::OnCanvasCursor(WPARAM wParam, LPARAM lParam)
{
    CString text;
    if (wParam != 0)
    {
        text.Format(L"%d, %dpx", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    m_statusBar.SetText(text, StatusCursor, 0);
    return 0;
}

void CMyPaintDlg::UpdateStatusBarParts()
{
    // 칸마다 오른쪽 끝 x. 마지막 칸(도구 설명)은 끝까지
    int rights[StatusPartCount] = { Scale(kStatusCursorRight), Scale(kStatusImageSizeRight),
                                    Scale(kStatusImageSizeRight + kStatusGhostWidth), -1 };
    m_statusBar.SetParts(StatusPartCount, rights);
}

void CMyPaintDlg::UpdateImageSizeStatus()
{
    const CSize size = m_canvas.GetImageSize();
    CString text;
    text.Format(L"%d × %dpx", size.cx, size.cy);
    m_statusBar.SetText(text, StatusImageSize, 0);
}
