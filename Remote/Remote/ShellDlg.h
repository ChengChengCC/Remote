#pragma once
#include "afxwin.h"
#include "IOCPServer.h"



// CShellDlg 对话框

class CShellDlg : public CDialog
{
	DECLARE_DYNAMIC(CShellDlg)

public:
	CShellDlg::CShellDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext);   // 标准构造函数
	virtual ~CShellDlg();

// 对话框数据
	enum { IDD = IDD_SHELL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit;

private:
	HICON m_hIcon;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	UINT m_nCurSel;
	UINT m_nReceiveLength;
public:
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);


	void CShellDlg::ResizeEdit(void);
	void OnReceiveComplete(void);   //有数据到达
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEdit();
	void CShellDlg::AddKeyBoardData(void);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
