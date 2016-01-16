// Talk.h: interface for the Talk class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TALK_H__590BF335_6097_4CA3_AD0B_DD1A418B5FED__INCLUDED_)
#define AFX_TALK_H__590BF335_6097_4CA3_AD0B_DD1A418B5FED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "Manager.h"

class Talk  :public CManager
{
public:
		Talk::Talk(CClientSocket *pClient);
	virtual ~Talk();


	static int CALLBACK DialogProc(HWND hDlg,unsigned int uMsg,
		WPARAM wParam,LPARAM lParam);


	static DWORD WINAPI CreateTalkDialog(LPVOID lparam); 


};

#endif // !defined(AFX_TALK_H__590BF335_6097_4CA3_AD0B_DD1A418B5FED__INCLUDED_)
