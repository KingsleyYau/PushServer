// MergeDBDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MergeDB.h"
#include "MergeDBDlg.h"

#undef T2W
#undef W2T
#include "TransString.h"


// SQL 语句

// 创建ServerPushMessage
LPCSTR	lpszCreateSRVPushMsg = TEXT("CREATE TABLE Server_PushMsg_db (\
								 [TokenID] nvarchar(64) NOT NULL\
								 ,[Body] nvarchar(1024) NOT NULL\
								 ,PRIMARY KEY (TokenID,Body) \
								 )");

// 创建ClientPushMessage
LPCSTR lpszCreateClientPushMsg = TEXT("CREATE TABLE Client_PushMsg_db (\
									  [TokenID] nvarchar(64) NOT NULL\
									  ,[Body] nvarchar(1024) NOT NULL\
									  ,PRIMARY KEY (TokenID,Body) \
									  )");

// 查找Server_PushMsg_db 表
LPCSTR lpszSelectServerPushMsg = TEXT("SELECT * FROM Server_PushMsg_db");

// 查找Client_PushMsg_db 表
LPCSTR lpszSelectClientPushMsg = TEXT("SELECT * FROM Client_PushMsg_db");

// 将记录插入Server_PushMsg_db 表
LPCSTR lpszInsertRec2Server_Push_db = TEXT("INSERT INTO Server_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");


// 将记录插入Client_PushMsg_db 表
LPCSTR lpszInsertRec2Client_Push_db = TEXT("INSERT INTO Client_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");

// 删除两个表中相同的记录Server_PushMsg_db,
LPCSTR lpszDeleteSameRecode = "delete from Server_PushMsg_db where exists "
													"(select * from Client_PushMsg_db where "
													"Server_PushMsg_db.TokenID = Client_PushMsg_db.TokenID "
													"and Server_PushMsg_db.Body = Client_PushMsg_db.Body) ";
/*
LPCSTR lpszDeleteSameRecode = "DELETE Server_PushMsg_db "
												  "FROM Server_PushMsg_db, "
												  "Client_PushMsg_db where "
												  "Server_PushMsg_db.TokenID = Client_PushMsg_db.TokenID ";*/

												//  "and Server_PushMsg_db.Body = Client_PushMsg_db.Body";

// 创建触发器，实现两表同步删除
LPCSTR lpszCreateTrigger = "create trigger _deleteRec_  delete on Server_PushMsg_db "
										" begin "
										"delete from Client_PushMsg_db "
										"where Client_PushMsg_db.TokenID = OLD.TokenID and "
										"Client_PushMsg_db.Body = OLD.Body;"
										"end;";
																	

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMergeDBDlg::CMergeDBDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMergeDBDlg::IDD, pParent)
	,m_bIsDBMerging(FALSE)
	,m_bIsDBDeleting(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMergeDBDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMergeDBDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SELECT_SRC_DIR, &CMergeDBDlg::OnBnClickedSelectSrcDir)
	ON_BN_CLICKED(IDC_SELECT_DEST_DIR, &CMergeDBDlg::OnBnClickedSelectDestDir)
	ON_BN_CLICKED(IDB_MERGE_DB, &CMergeDBDlg::OnBnClickedMergeDb)
	ON_BN_CLICKED(IDB_DELETE_SAME_REC, &CMergeDBDlg::OnBnClickedDeleteSameRec)
	ON_MESSAGE(WM_MERGE_DB_DONE, &CMergeDBDlg::OnMergeDBDone)
	ON_MESSAGE(WM_DELETE_SAMEREC_DONE, &CMergeDBDlg::OnDeleteSameRecDone)
END_MESSAGE_MAP()


// CMergeDBDlg 消息处理程序

BOOL CMergeDBDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMergeDBDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMergeDBDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
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
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CMergeDBDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 选择源数据库根目录
void CMergeDBDlg::OnBnClickedSelectSrcDir()
{
	char szDbSrcPath[MAX_PATH] = {0};
	char szDefaultPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDefaultPath, _countof(szDefaultPath));
	PathRemoveFileSpec(szDefaultPath);
	BROWSEINFO bi = {0};
	bi.hwndOwner	    = m_hWnd;
	bi.pidlRoot		    = NULL;
	bi.lpszTitle			    = TEXT("请选择数据库的根目录！");
	bi.pszDisplayName= szDbSrcPath;
	bi.ulFlags				= BIF_EDITBOX;
	bi.lParam				= (LPARAM)szDefaultPath;
	bi.lpfn					= BrowseCallbackProc ;
	LPITEMIDLIST pItemDList = SHBrowseForFolder(&bi);

	if (pItemDList)
	{
		if (SHGetPathFromIDList(pItemDList, szDbSrcPath))
		{
			m_strDBSrcRootPath = szDbSrcPath;
		}
		// 释放内存
		IMalloc *imalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pItemDList);
			imalloc->Release();
		}
	}
	SetDlgItemText(IDE_DB_SRC_PATH, m_strDBSrcRootPath.c_str());
}
// 选择目标数据库输出目录
void CMergeDBDlg::OnBnClickedSelectDestDir()
{
	char szDbDestPath[MAX_PATH] = {0};
	char szDefaultPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDefaultPath, _countof(szDefaultPath));
	BROWSEINFO bi = {0};
	bi.hwndOwner	    = m_hWnd;
	bi.pidlRoot		    = NULL;
	bi.lpszTitle			    = TEXT("请选择目标数据库的输出目录！");
	bi.pszDisplayName= szDbDestPath;
	bi.ulFlags				= BIF_EDITBOX;
	bi.lpfn					= NULL;
	bi.lParam				= NULL;
	LPITEMIDLIST pItemDList = SHBrowseForFolder(&bi);
	if (pItemDList)
	{
		if (SHGetPathFromIDList(pItemDList, szDbDestPath))
		{
			m_strDBDestPath= szDbDestPath;
		}
		// 释放内存
		IMalloc *imalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pItemDList);
			imalloc->Release();
		}
	}
	SetDlgItemText(IDE_DB_DEST_PATH, m_strDBDestPath.c_str());
}

void CMergeDBDlg::OnBnClickedMergeDb()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_strDBSrcRootPath.empty() || m_strDBDestPath.empty() )
	{
		MessageBox(TEXT("数据库源目录和合并输出目录不能为空！"),
							TEXT("提示！"),
							MB_OK | MB_ICONWARNING);
		return;
	}
	m_bIsDBMerging = TRUE;			// 合并标志置为 TRUE;
	// 合并数据库时，不能进行删除记录的操作
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(FALSE);

	GetDlgItem(IDB_MERGE_DB)->EnableWindow(FALSE);
	SetDlgItemText(IDB_MERGE_DB,TEXT("合并中…"));
	m_hTreadMergeDB = ::CreateThread(NULL, 0, _ThreadMergeDB, this, 0, NULL);

}

void CMergeDBDlg::OnBnClickedDeleteSameRec()
{
	// 删除数据库记录时，不能进行合并数据库的操作
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(FALSE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(FALSE);
	SetDlgItemText(IDB_DELETE_SAME_REC,TEXT("删除中…"));
	m_hTreadDeleteSameRecord = ::CreateThread(NULL, 0, _ThreadDeleteSameRecord,
																		this, 0, NULL);	
}

BOOL CMergeDBDlg::SearchDBFile(LPCTSTR lpRootPath)
{
	string strFindPath = lpRootPath;
	strFindPath += "\\*.*";
	CFileFind Finder;
	BOOL bFound;											// 判断是否找到文件
	bFound = Finder.FindFile(strFindPath.c_str());
	string strTempFolder;	
	while( bFound)
	{
		bFound = Finder.FindNextFile();
		if (Finder.IsDots())
		{
			continue;
		}
		if ( Finder.IsDirectory() )							// 找到的是文件夹，则遍历该文件夹下的文件
		{
			strTempFolder.clear();
			strTempFolder = Finder.GetFilePath();
			SearchDBFile(strTempFolder.c_str());
		} 
		else
		{
			if (0 == _tcscmp(::PathFindExtension(Finder.GetFileName()), TEXT(".db")))
			{
				m_DBPathVect.push_back((LPCSTR)Finder.GetFilePath());
			}
		}
	}
	Finder.Close();
	return TRUE;
}
BOOL CMergeDBDlg::MergeDB()
{
	ASSERT(!m_DBPathVect.empty());
	ASSERT(!m_strDBDestPath.empty());

	string strDBDestPath = m_strDBDestPath;
	strDBDestPath += TEXT("\\merge.db");
	
	// 先创建目标数据库和当中的表

	int nRet = sqlite3_open(TransString::ascII2utf8(strDBDestPath).c_str(), &m_pTargetDB);
//	int nRet = sqlite3_open(strDBDestPath.c_str(), &m_pTargetDB);					// 2012-12-26 Modified by Sanwen
	if (nRet != SQLITE_OK)
	{
		OutputDebugString(_T("合并数据库时，数据库打开或创建失败\n"));
		return FALSE;
	} 

	// 创建Server_PushMsg_db
	sqlite3_exec(m_pTargetDB, lpszCreateSRVPushMsg, NULL, NULL, NULL);
	sqlite3_exec(m_pTargetDB, lpszCreateClientPushMsg, NULL, NULL, NULL);

	char *errmsg = NULL;
	nRet = sqlite3_exec(m_pTargetDB, "BEGIN;", NULL, NULL, &errmsg);
//////////////////////////////////////////////////////////////////////////

	for (int i=0; i<m_DBPathVect.size(); ++i)
	{
		vector<string>::iterator iter = m_DBPathVect.begin()+i;
		sqlite3 *pDB;
		char *errmsg;
		int nRet = sqlite3_open(TransString::ascII2utf8(*iter).c_str(), &pDB);
	//	int nRet = sqlite3_open(iter->c_str(), &pDB);   // 2012-12-26 Modified by Sanwen

		if (SQLITE_OK == sqlite3_exec(pDB, lpszSelectServerPushMsg, NULL, NULL, &errmsg) )
		{
			// 此数据库中含有 ServerPushMsg_db 表
			int nRet= sqlite3_exec(pDB, lpszSelectServerPushMsg, ServerPushMsg_Callback, this, NULL);
		}
		if (SQLITE_OK == sqlite3_exec(pDB, lpszSelectClientPushMsg, NULL, NULL, &errmsg) )
		{
			// 此数据库中含有 ClientPushMsg_db 表
			int nRet = sqlite3_exec(pDB, lpszSelectClientPushMsg, ClientPushMsg_Callback, this, NULL);
		}

	}
//////////////////////////////////////////////////////////////////////////
	sqlite3_exec(m_pTargetDB, "COMMIT;", NULL, NULL, &errmsg);
	 sqlite3_close(m_pTargetDB);
	return TRUE;
}
// 合并数据库线程
DWORD WINAPI CMergeDBDlg::_ThreadMergeDB(LPVOID lParam)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(lParam);
	DWORD dwRet = 0;
	pDlg->SearchDBFile(pDlg->m_strDBSrcRootPath.c_str() );
	pDlg->MergeDB();

	// 完成，通知主线程
	pDlg->PostMessage(WM_MERGE_DB_DONE);
	return dwRet;
}

// 删除数据库中相同记录线程
DWORD WINAPI CMergeDBDlg::_ThreadDeleteSameRecord(LPVOID lParam)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(lParam);
	DWORD dwRet = 0;
	string strDBFile = pDlg->m_strDBDestPath;
	strDBFile += TEXT("\\merge.db");

	sqlite3_open(TransString::ascII2utf8(strDBFile).c_str(), &pDlg->m_pTargetDB);
	//sqlite3_open(strDBFile.c_str(), &pDlg->m_pTargetDB);
	char *errmsg = NULL;
	int nRet = sqlite3_exec(pDlg->m_pTargetDB, lpszCreateTrigger, NULL, NULL, &errmsg);
	nRet = sqlite3_exec(pDlg->m_pTargetDB, "BEGIN;", NULL, NULL, &errmsg);
	nRet = sqlite3_exec(pDlg->m_pTargetDB, lpszDeleteSameRecode, NULL, NULL,&errmsg);
	nRet = sqlite3_exec(pDlg->m_pTargetDB, "COMMIT;", NULL, NULL, &errmsg);
	sqlite3_close(pDlg->m_pTargetDB);
	pDlg->PostMessage(WM_DELETE_SAMEREC_DONE);
	return dwRet;	
}

LRESULT CMergeDBDlg::OnMergeDBDone(WPARAM wParam, LPARAM lParam)					// 数据库合并已经完成
{
	m_bIsDBMerging = FALSE;					// 合并标志置为 FALSE
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(TRUE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(TRUE);
	SetDlgItemText(IDB_MERGE_DB,TEXT("合并数据库"));
	return 1;
}
LRESULT CMergeDBDlg::OnDeleteSameRecDone(WPARAM wParam, LPARAM lParam)			// 删除相同记录已经完成
{
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(TRUE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(TRUE);
	SetDlgItemText(IDB_DELETE_SAME_REC,TEXT("删除相同记录"));

	return 1;
}

// 文件夹浏览对话框回调函数，指定缺省目录
int CALLBACK CMergeDBDlg::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (BFFM_INITIALIZED == uMsg)
	{
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}

// Sqlite 回调 ,将读到的记录插入目标数据库的 Server_PushMsg_db 表中
int CMergeDBDlg::ServerPushMsg_Callback(void* pvoid, int argc, char** argv, char** col)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(pvoid);
	char szSql[256] = {0};
	sprintf_s(szSql, lpszInsertRec2Server_Push_db, argv[0], argv[1]);
	sqlite3_exec(pDlg->m_pTargetDB, szSql, NULL, NULL, NULL);

	return 0;
}

// Sqlite 回调 ,将读到的记录插入目标数据库的 Client_PushMsg_db 表中
int CMergeDBDlg::ClientPushMsg_Callback(void* pvoid, int argc, char** argv, char** col)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(pvoid);
	char szSql[256] = {0};
	sprintf_s(szSql, lpszInsertRec2Client_Push_db, argv[0], argv[1]);
	sqlite3_exec(pDlg->m_pTargetDB, szSql, NULL, NULL, NULL);

	return 0;
}