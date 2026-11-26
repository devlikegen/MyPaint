/**
 * @file MyPaint.cpp
 * @brief 애플리케이션 클래스 구현
 */
#include "pch.h"
#include "MyPaint.h"
#include "MyPaintDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    /** @brief 수명 동안 GDI+를 켜 두는 RAII 객체 */
    class GdiplusSession
    {
    public:
        GdiplusSession()
        {
            Gdiplus::GdiplusStartupInput input;
            m_started = Gdiplus::GdiplusStartup(&m_token, &input, nullptr) == Gdiplus::Ok;
        }
        ~GdiplusSession()
        {
            if (m_started)
            {
                Gdiplus::GdiplusShutdown(m_token);
            }
        }
        GdiplusSession(const GdiplusSession&) = delete;
        GdiplusSession& operator=(const GdiplusSession&) = delete;

        bool IsStarted() const { return m_started; }

    private:
        ULONG_PTR m_token = 0;
        bool m_started = false;
    };
}

BEGIN_MESSAGE_MAP(CMyPaintApp, CWinApp)
END_MESSAGE_MAP()

/** @brief 유일한 애플리케이션 개체 */
CMyPaintApp theApp;

CMyPaintApp::CMyPaintApp()
{
}

BOOL CMyPaintApp::InitInstance()
{
    // 공용 컨트롤 6.0(비주얼 스타일)을 쓰려면 InitCommonControlsEx가 필요합니다.
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    CWinApp::InitInstance();

    // GDI+는 대화 상자보다 먼저 켜고 나중에 꺼야 하므로, 대화 상자보다 먼저 선언합니다.
    GdiplusSession gdiplus;
    if (!gdiplus.IsStarted())
    {
        AfxMessageBox(L"GDI+를 초기화하지 못했습니다.", MB_ICONERROR);
        return FALSE;
    }

    CMyPaintDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // 대화 상자가 닫혔으므로 메시지 펌프를 시작하지 않고 종료합니다.
    return FALSE;
}
