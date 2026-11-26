/**
 * @file PaintDocument.cpp
 * @brief 그림 문서 구현
 */
#include "pch.h"
#include "PaintDocument.h"
#include "ImageFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPaintDocument::CPaintDocument(int untitledNumber) : m_untitledNumber(untitledNumber)
{
}

bool CPaintDocument::Create(int width, int height)
{
    return m_surface.Create(width, height, RGB(255, 255, 255));
}

bool CPaintDocument::LoadFromFile(const CString& path, CString& errorMessage)
{
    if (!LoadImageFile(path, m_surface, errorMessage))
    {
        return false;
    }
    m_filePath = path;
    m_modified = false;
    m_scroll = CPoint(0, 0);
    return true;
}

CString CPaintDocument::GetTitle() const
{
    if (!m_filePath.IsEmpty())
    {
        return CString(::PathFindFileName(m_filePath));
    }
    CString title(L"제목 없음");
    if (m_untitledNumber > 1)
    {
        title.AppendFormat(L" %d", m_untitledNumber);
    }
    return title;
}
