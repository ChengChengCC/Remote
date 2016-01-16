#pragma once

#include "IOCPServer.h"
#include "Common.h"
// CScreenSpyDlg 对话框

class CScreenSpyDlg : public CDialog
{
	DECLARE_DYNAMIC(CScreenSpyDlg)

public:
	CScreenSpyDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);  // 标准构造函数
	virtual ~CScreenSpyDlg();

	void CScreenSpyDlg::InitMMI(void);
	void CScreenSpyDlg::DrawFirstScreen(void);
	void CScreenSpyDlg::SendNext(void);
	void CScreenSpyDlg::OnReceiveComplete(void);
	void CScreenSpyDlg::DrawTipString(CString str);
	void CScreenSpyDlg::DrawNextScreenDiff(void);
	void CScreenSpyDlg::SendCommand(MSG* pMsg);
	bool CScreenSpyDlg::SaveSnapshot(void);
	void CScreenSpyDlg::UpdateLocalClipboard(char *buf, int len);
	void CScreenSpyDlg::SendLocalClipboard(void);


private:
	int	m_nBitCount;
	bool m_bIsFirst;
	bool m_bIsTraceCursor;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;
	HICON m_hIcon;
	MINMAXINFO m_MMI;                   //为了控制窗口的大小，在窗口初始化时，需要用到MINMAXINFO结构体
	HDC m_hDC, m_hMemDC, m_hPaintDC;
	HBITMAP	m_hFullBitmap;
	LPVOID m_lpScreenDIB;
	LPBITMAPINFO m_lpbmi, m_lpbmi_rect;
	UINT m_nCount;
	UINT m_HScrollPos, m_VScrollPos;
	HCURSOR	m_hRemoteCursor;
	DWORD	m_dwCursor_xHotspot, m_dwCursor_yHotspot;
	POINT	m_RemoteCursorPos;
	BYTE	m_bCursorIndex;
	CCursorInfo	m_CursorInfo;    //自定义的一个系统的光标类
	bool m_bIsCtrl;

// 对话框数据
	enum { IDD = IDD_SCREENSPY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
