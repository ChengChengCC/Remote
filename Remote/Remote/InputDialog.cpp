// InputDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "InputDialog.h"
#include "afxdialogex.h"


// CInputDialog 对话框

IMPLEMENT_DYNAMIC(CInputDialog, CDialog)

CInputDialog::CInputDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInputDialog::IDD, pParent)
	, m_str(_T(""))
{

}

CInputDialog::~CInputDialog()
{
}

void CInputDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_INPUT, m_str);
}


BEGIN_MESSAGE_MAP(CInputDialog, CDialog)
	ON_BN_CLICKED(IDOK, &CInputDialog::OnBnClickedOk)
END_MESSAGE_MAP()


// CInputDialog 消息处理程序


void CInputDialog::OnBnClickedOk()
{
	UpdateData(TRUE);
	if (m_str.IsEmpty()) {
		MessageBeep(0);
		return;   //不关闭对话框
	}
	CDialog::OnOK();
}
