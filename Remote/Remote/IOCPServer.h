
#pragma once
#include "CpuUseage.h"
#include "Buffer.h"

#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")




enum IOType 
{
	IOInitialize,
	IORead,
	IOWrite,
	IOIdle
};


#define	NC_CLIENT_CONNECT		0x0001
#define	NC_CLIENT_DISCONNECT	0x0002
#define	NC_TRANSMIT				0x0003
#define	NC_RECEIVE				0x0004
#define NC_RECEIVE_COMPLETE		0x0005 // 完整接收

#define HDR_SIZE	13
#define FLAG_SIZE	5
#define HUERISTIC_VALUE 2

#define SIO_KEEPALIVE_VALS    _WSAIOW(IOC_VENDOR,4)

struct tcp_keepalive {  //套接字的保活机制
	ULONG onoff;
	ULONG keepalivetime;
	ULONG keepaliveinterval;
};


struct ClientContext     //定义的客户上下背景文
{
	 SOCKET				m_Socket;                   // 套接字(与一个信号进行通信使用的)
	 CBuffer		    m_CompressionBuffer;	    // 接收到的压缩的数据
	 CBuffer			m_DeCompressionBuffer;	    // 解压后的数据
	 CBuffer			m_ResendWriteBuffer;	    // 上次发送的数据包，接收失败时重发时用 也就是备份的数据
	 CBuffer			m_WriteBuffer;              // 向被控端发送的数据
	 WSABUF				m_wsaInBuffer;              // 这里没有办法因为IOCP系列的函数都要求使用WSABUF
	 BYTE				m_byInBuffer[8192];
	 BOOL				m_bIsMainSocket;            // 是主socket 用户

	 LONG				m_nMsgIn;
	 LONG				m_nMsgOut;
	 WSABUF				m_wsaOutBuffer;
	 int				m_Dialog[2]; // 放对话框列表用，第一个int是类型是否有Dlg，第二个是CDialog的地址那个Dlg
};

typedef void (CALLBACK* NOTIFYPROC)(LPVOID, ClientContext*, UINT nCode);

typedef CList<ClientContext*, ClientContext* > ContextList;                

class CMainFrame;




#include "Mapper.h"
class CIOCPServer
{
public:
	NOTIFYPROC					m_pNotifyProc;
	CMainFrame*					m_pFrame;                    //主对话框


	LONG					    m_nCurrentThreads;
	LONG					    m_nBusyThreads;
	bool					    m_bTimeToKill;
	UINT					    m_nMaxConnections;          // 最大连接数

	CIOCPServer();
	virtual ~CIOCPServer();         //套接字库的的Clearup

	void CIOCPServer::Shutdown();   //资源的释放 套接字的关闭

	bool CIOCPServer::Initialize(NOTIFYPROC pNotifyProc, CMainFrame* pFrame, int nMaxConnections, int nPort);
	static unsigned __stdcall ListenThreadProc(LPVOID lpVoid);   //注意在类中定义线程回调时一定要使用我们Static 
	static unsigned __stdcall ThreadPoolFunc(LPVOID WorkContext);
	bool CIOCPServer::InitializeIOCP(void);                      //IOCP完成端口的创建
	void CIOCPServer::OnAccept();								 //当有链接信号被我们的监听线程捕获
	BOOL CIOCPServer::AssociateSocketWithCompletionPort(SOCKET socket, HANDLE hCompletionPort, DWORD dwCompletionKey);   //这里我们将用户的上下背景文当成了完成Key
	static CRITICAL_SECTION	m_cs;
	ClientContext*  CIOCPServer::AllocateContext();
	void CIOCPServer::MoveToFreePool(ClientContext *pContext);
	void CIOCPServer::RemoveStaleClient(ClientContext* pContext, BOOL bGraceful);     //完成端口失败 移除该客户上下背景文

	void CIOCPServer::PostRecv(ClientContext* pContext);

	void CIOCPServer::Send(ClientContext* pContext, LPBYTE lpData, UINT nSize);
protected:
	WSAEVENT				m_hEvent;
	SOCKET					m_sockListen;   
	HANDLE					m_hThread;                  //监听线程的线程句柄
	HANDLE					m_hKillEvent;               //要求监听线程退出的事件请求     在构造函数中要记住创建  
	bool                    m_bInit;
	HANDLE					m_hCompletionPort;          //完成端口

	//线程池
	LONG					m_nThreadPoolMin;
	LONG					m_nThreadPoolMax;
	LONG					m_nCPULowThreadshold;
	LONG					m_nCPUHighThreadshold;

	CCpuUsage				m_cpu;
	LONG				    m_nWorkerCnt;               //我们自己向完成端口投递的线程个数的预想值


	LONG					m_nKeepLiveTime;            //保活机制使用

	//内存池
	ContextList				m_listContexts;             //两个链表的头指针      
	ContextList				m_listFreePool;
	
	
	UINT					m_nSendKbps;                // 发送即时速度
	UINT					m_nRecvKbps;                // 接受即时速度
	BYTE				    m_bPacketFlag[5];           // 数据包的头部 也就是Shine
	
	


	BEGIN_IO_MSG_MAP()
		IO_MESSAGE_HANDLER(IOInitialize, OnClientInitializing)
		IO_MESSAGE_HANDLER(IORead, OnClientReading)                   //被控端向主控制发送数据都被这里响应接受
		IO_MESSAGE_HANDLER(IOWrite, OnClientWriting)                  //主控端向被控端发送数据在这里得到响应
	END_IO_MSG_MAP()

	bool OnClientInitializing	(ClientContext* pContext, DWORD dwSize = 0);
	bool OnClientReading		(ClientContext* pContext, DWORD dwSize = 0);
	bool OnClientWriting		(ClientContext* pContext, DWORD dwSize = 0);



	

};




//我们在这里还建立了一个辅助的类 用于 多线的数据的同步

class CLock
{
public:
	CLock(CRITICAL_SECTION& cs, const CString& strFunc)
	{
		m_strFunc = strFunc;
		m_pcs = &cs;
		Lock();
	}
	~CLock()
	{
		Unlock();

	}

	void Unlock()
	{
		LeaveCriticalSection(m_pcs);
		TRACE(_T("LC %d %s\n") , GetCurrentThreadId() , m_strFunc);
	}

	void Lock()
	{
		TRACE(_T("EC %d %s\n") , GetCurrentThreadId(), m_strFunc);
		EnterCriticalSection(m_pcs);
	}


protected:
	CRITICAL_SECTION*	m_pcs;
	CString				m_strFunc;

};






class OVERLAPPEDPLUS 
{
public:
	OVERLAPPED			m_ol;
	IOType				m_ioType;

	OVERLAPPEDPLUS(IOType ioType) {
		ZeroMemory(this, sizeof(OVERLAPPEDPLUS));
		m_ioType = ioType;
	}
};




