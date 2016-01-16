// ServerManager.h: interface for the CServerManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERMANAGER_H__58AFCDDE_0DDD_414E_8283_511D6401FF67__INCLUDED_)
#define AFX_SERVERMANAGER_H__58AFCDDE_0DDD_414E_8283_511D6401FF67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"

class CServerManager : public CManager  
{
public:
	CServerManager(CClientSocket *pClient);
	virtual ~CServerManager();
	void CServerManager::SendServicesList();
	
	LPBYTE CServerManager::getServerList();

};

#endif // !defined(AFX_SERVERMANAGER_H__58AFCDDE_0DDD_414E_8283_511D6401FF67__INCLUDED_)
