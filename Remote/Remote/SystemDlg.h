#pragma once

#include "IOCPServer.h"
#include "afxcmn.h"


// CSystemDlg 对话框

class CSystemDlg : public CDialog
{
	DECLARE_DYNAMIC(CSystemDlg)

public:
	CSystemDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);    // 标准构造函数
	virtual ~CSystemDlg();

	void OnReceiveComplete(void);
	void CSystemDlg::ShowWindowsList(void);
	void CSystemDlg::ShowProcessList(void);
	void CSystemDlg::GetProcessList(void);
	void CSystemDlg::GetWindowsList(void);

	void CSystemDlg::AdjustList(void);

// 对话框数据
	enum { IDD = IDD_SYSTEM };

private:
	HICON m_hIcon;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	BOOL m_bHow;     //用来区分窗口管理和进程管理

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list_process;
	virtual BOOL OnInitDialog();
	afx_msg void OnKillprocess();
	afx_msg void OnNMRClickListProcess(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRefreshpslist();
	afx_msg void OnWindowReflush();
	afx_msg void OnWindowClose();
	afx_msg void OnWindowHide();
	afx_msg void OnWindowReturn();
	afx_msg void OnWindowMax();
	afx_msg void OnWindowMin();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
