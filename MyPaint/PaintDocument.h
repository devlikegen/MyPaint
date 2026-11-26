#pragma once

/**
 * @file PaintDocument.h
 * @brief 탭 하나가 가지는 그림 문서
 */

#include "CanvasSurface.h"

/**
 * @brief 그림 문서. 탭마다 하나씩 있고, 탭끼리 공유하지 않는 상태를 모두 담습니다.
 *
 * - 그림 데이터
 * - 저장 경로 (아직 저장하지 않았으면 빈 문자열)
 * - 마지막 저장 이후 바뀌었는지
 * - 캔버스 스크롤 위치 (탭을 바꿨다 돌아와도 보던 위치 유지)
 *
 * 도구, 굵기, 채우기 방식, 색은 문서가 아니라 캔버스 창이 가지므로 모든 탭이 공유합니다.
 */
class CPaintDocument
{
public:
    /**
     * @brief 생성자
     * @param untitledNumber 저장 전 제목에 붙일 번호 ("제목 없음 2")
     */
    explicit CPaintDocument(int untitledNumber);

    CPaintDocument(const CPaintDocument&) = delete;
    CPaintDocument& operator=(const CPaintDocument&) = delete;

    /**
     * @brief 지정한 크기의 흰 그림을 만듭니다.
     * @return 성공하면 true
     */
    bool Create(int width, int height);

    /**
     * @brief 이미지 파일을 불러와 이 문서의 그림으로 만듭니다. 성공하면 그 파일이 저장 경로가 됩니다.
     * @param path 파일 경로
     * @param errorMessage [out] 실패했을 때 사용자에게 보여 줄 이유
     * @return 성공하면 true
     */
    bool LoadFromFile(const CString& path, CString& errorMessage);

    /** @brief 새로 만든 뒤 아무것도 그리거나 저장하지 않은 빈 문서면 true (불러온 파일로 바꿔 써도 됨) */
    bool IsPristine() const { return m_filePath.IsEmpty() && !m_modified; }

    /** @brief 그림 데이터 */
    CCanvasSurface& Surface() { return m_surface; }

    /** @brief 탭과 창 제목에 쓸 이름. 저장했으면 파일 이름, 아니면 "제목 없음 N" */
    CString GetTitle() const;

    /** @brief 저장 경로. 아직 저장하지 않았으면 빈 문자열 */
    const CString& GetFilePath() const { return m_filePath; }
    void SetFilePath(const CString& path) { m_filePath = path; }

    /** @brief 마지막 저장 이후 바뀌었으면 true */
    bool IsModified() const { return m_modified; }
    void SetModified(bool modified) { m_modified = modified; }

    /** @brief 캔버스 스크롤 위치 */
    CPoint GetScroll() const { return m_scroll; }
    void SetScroll(CPoint scroll) { m_scroll = scroll; }

private:
    CCanvasSurface m_surface;
    CString m_filePath;
    bool m_modified = false;
    CPoint m_scroll;
    int m_untitledNumber = 1;
};
