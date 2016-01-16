// FileTransferModeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "FileTransferModeDlg.h"
#include "afxdialogex.h"


// CFileTransferModeDlg 对话框

IMPLEMENT_DYNAMIC(CFileTransferModeDlg, CDialog)

CFileTransferModeDlg::CFileTransferModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFileTransferModeDlg::IDD, pParent)
{

}

CFileTransferModeDlg::~CFileTransferModeDlg()
{
}

void CFileTransferModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFileTransferModeDlg, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_OVERWRITE, IDC_JUMP_ALL, OnEndDialog)
END_MESSAGE_MAP()


// CFileTransferModeDlg 消息处理程序

void CFileTransferModeDlg::OnEndDialog(UINT id)
{
	// TODO: Add your control notification handler code here
	EndDialog(id);
}

BOOL CFileTransferModeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	CString	str;
	str.Format("此文件夹已包含一个名为“%s”的文件", m_strFileName);

	for (int i = 0; i < str.GetLength(); i += 120)
	{
		str.Insert(i, "\n");
		i += 1;
	}

	SetDlgItemText(IDC_TIP, str);
	return TRUE;

	return TRUE;  
}




