#pragma once

#include "IOCPServer.h"
#include "Audio.h"
// CAudioDlg 对话框

class CAudioDlg : public CDialog
{
	DECLARE_DYNAMIC(CAudioDlg)

public:
	CAudioDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL) ;   // 标准构造函
	virtual ~CAudioDlg();

	void CAudioDlg::OnReceiveComplete(void);

// 对话框数据
	enum { IDD = IDD_AUDIO };
public:
	bool m_bIsWorking;
	CAudio m_Audio;
private:

	UINT m_nTotalRecvBytes;
	HICON m_hIcon;
	HANDLE	m_hWorkThread;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bIsSendLocalAudio;
	virtual BOOL OnInitDialog();
	static DWORD  WorkThread(LPVOID lparam);      
	afx_msg void OnClose();
	afx_msg void OnBnClickedSendLocalaudio();
};
