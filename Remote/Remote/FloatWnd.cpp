// FloatWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "FloatWnd.h"
#include "afxdialogex.h"


// CFloatWnd 对话框

IMPLEMENT_DYNAMIC(CFloatWnd, CDialog)

CFloatWnd::CFloatWnd(CWnd* pParent /*=NULL*/)
	: CDialog(CFloatWnd::IDD, pParent)
{

}

CFloatWnd::~CFloatWnd()
{
}

void CFloatWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGO, m_Logo);
}


BEGIN_MESSAGE_MAP(CFloatWnd, CDialog)
END_MESSAGE_MAP()


// CFloatWnd 消息处理程序


BOOL CFloatWnd::OnInitDialog()
{
	CDialog::OnInitDialog();

	CBitmap m_Bitmap;
	HBITMAP hBitmap = m_Logo.GetBitmap();
	ASSERT(hBitmap);

	m_Bitmap.Attach(hBitmap);
	BITMAP bmp;
	m_Bitmap.GetBitmap(&bmp);

	int nX = bmp.bmWidth;
	int nY = bmp.bmHeight;

	
	m_Logo.MoveWindow(0,0,nX,nY);
	CenterWindow();

	m_Bitmap.Detach();

	//加入WS_EX_LAYERED扩展属性
	SetWindowLong(m_hWnd,GWL_EXSTYLE,GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)^0x80000);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}



void CFloatWnd::OnUpdateTransparent(int iTransparent)
{
    HINSTANCE hInst = LoadLibrary("User32.DLL");
	if(hInst)
	{
		typedef BOOL (WINAPI *SLWA)(HWND,COLORREF,BYTE,DWORD);
		SLWA pFun = NULL;
		//取得SetLayeredWindowAttributes函数指针 
		pFun = (SLWA)GetProcAddress(hInst,"SetLayeredWindowAttributes");
		if(pFun)
		{
			pFun(m_hWnd,255,255,0);
		}
		FreeLibrary(hInst); 
	}
}