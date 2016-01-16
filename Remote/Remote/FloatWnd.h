#pragma once
#include "afxwin.h"


// CFloatWnd 对话框

class CFloatWnd : public CDialog
{
	DECLARE_DYNAMIC(CFloatWnd)

public:
	CFloatWnd(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFloatWnd();

// 对话框数据
	enum { IDD = IDD_DIALOG_FLOAT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void CFloatWnd::OnUpdateTransparent(int iTransparent);
	CStatic m_Logo;
};
