#pragma once

#include "IOCPServer.h"
#include "afxcmn.h"

// CServerDlg 对话框

class CServerDlg : public CDialog
{
	DECLARE_DYNAMIC(CServerDlg)

public:
	CServerDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CServerDlg();


	int CServerDlg::ShowServiceList(void);

// 对话框数据
	enum { IDD = IDD_SERVERDLG };

	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;

	void OnReceiveComplete(void);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	virtual BOOL OnInitDialog();
};
