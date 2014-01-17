// MergeDBDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MergeDB.h"
#include "MergeDBDlg.h"

#undef T2W
#undef W2T
#include "TransString.h"


// SQL ���

// ����ServerPushMessage
LPCSTR	lpszCreateSRVPushMsg = TEXT("CREATE TABLE Server_PushMsg_db (\
								 [TokenID] nvarchar(64) NOT NULL\
								 ,[Body] nvarchar(1024) NOT NULL\
								 ,PRIMARY KEY (TokenID,Body) \
								 )");

// ����ClientPushMessage
LPCSTR lpszCreateClientPushMsg = TEXT("CREATE TABLE Client_PushMsg_db (\
									  [TokenID] nvarchar(64) NOT NULL\
									  ,[Body] nvarchar(1024) NOT NULL\
									  ,PRIMARY KEY (TokenID,Body) \
									  )");

// ����Server_PushMsg_db ��
LPCSTR lpszSelectServerPushMsg = TEXT("SELECT * FROM Server_PushMsg_db");

// ����Client_PushMsg_db ��
LPCSTR lpszSelectClientPushMsg = TEXT("SELECT * FROM Client_PushMsg_db");

// ����¼����Server_PushMsg_db ��
LPCSTR lpszInsertRec2Server_Push_db = TEXT("INSERT INTO Server_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");


// ����¼����Client_PushMsg_db ��
LPCSTR lpszInsertRec2Client_Push_db = TEXT("INSERT INTO Client_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");

// ɾ������������ͬ�ļ�¼Server_PushMsg_db,
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

// ������������ʵ������ͬ��ɾ��
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


// CMergeDBDlg ��Ϣ�������

BOOL CMergeDBDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CMergeDBDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CMergeDBDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ѡ��Դ���ݿ��Ŀ¼
void CMergeDBDlg::OnBnClickedSelectSrcDir()
{
	char szDbSrcPath[MAX_PATH] = {0};
	char szDefaultPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDefaultPath, _countof(szDefaultPath));
	PathRemoveFileSpec(szDefaultPath);
	BROWSEINFO bi = {0};
	bi.hwndOwner	    = m_hWnd;
	bi.pidlRoot		    = NULL;
	bi.lpszTitle			    = TEXT("��ѡ�����ݿ�ĸ�Ŀ¼��");
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
		// �ͷ��ڴ�
		IMalloc *imalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pItemDList);
			imalloc->Release();
		}
	}
	SetDlgItemText(IDE_DB_SRC_PATH, m_strDBSrcRootPath.c_str());
}
// ѡ��Ŀ�����ݿ����Ŀ¼
void CMergeDBDlg::OnBnClickedSelectDestDir()
{
	char szDbDestPath[MAX_PATH] = {0};
	char szDefaultPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDefaultPath, _countof(szDefaultPath));
	BROWSEINFO bi = {0};
	bi.hwndOwner	    = m_hWnd;
	bi.pidlRoot		    = NULL;
	bi.lpszTitle			    = TEXT("��ѡ��Ŀ�����ݿ�����Ŀ¼��");
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
		// �ͷ��ڴ�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (m_strDBSrcRootPath.empty() || m_strDBDestPath.empty() )
	{
		MessageBox(TEXT("���ݿ�ԴĿ¼�ͺϲ����Ŀ¼����Ϊ�գ�"),
							TEXT("��ʾ��"),
							MB_OK | MB_ICONWARNING);
		return;
	}
	m_bIsDBMerging = TRUE;			// �ϲ���־��Ϊ TRUE;
	// �ϲ����ݿ�ʱ�����ܽ���ɾ����¼�Ĳ���
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(FALSE);

	GetDlgItem(IDB_MERGE_DB)->EnableWindow(FALSE);
	SetDlgItemText(IDB_MERGE_DB,TEXT("�ϲ��С�"));
	m_hTreadMergeDB = ::CreateThread(NULL, 0, _ThreadMergeDB, this, 0, NULL);

}

void CMergeDBDlg::OnBnClickedDeleteSameRec()
{
	// ɾ�����ݿ��¼ʱ�����ܽ��кϲ����ݿ�Ĳ���
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(FALSE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(FALSE);
	SetDlgItemText(IDB_DELETE_SAME_REC,TEXT("ɾ���С�"));
	m_hTreadDeleteSameRecord = ::CreateThread(NULL, 0, _ThreadDeleteSameRecord,
																		this, 0, NULL);	
}

BOOL CMergeDBDlg::SearchDBFile(LPCTSTR lpRootPath)
{
	string strFindPath = lpRootPath;
	strFindPath += "\\*.*";
	CFileFind Finder;
	BOOL bFound;											// �ж��Ƿ��ҵ��ļ�
	bFound = Finder.FindFile(strFindPath.c_str());
	string strTempFolder;	
	while( bFound)
	{
		bFound = Finder.FindNextFile();
		if (Finder.IsDots())
		{
			continue;
		}
		if ( Finder.IsDirectory() )							// �ҵ������ļ��У���������ļ����µ��ļ�
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
	
	// �ȴ���Ŀ�����ݿ�͵��еı�

	int nRet = sqlite3_open(TransString::ascII2utf8(strDBDestPath).c_str(), &m_pTargetDB);
//	int nRet = sqlite3_open(strDBDestPath.c_str(), &m_pTargetDB);					// 2012-12-26 Modified by Sanwen
	if (nRet != SQLITE_OK)
	{
		OutputDebugString(_T("�ϲ����ݿ�ʱ�����ݿ�򿪻򴴽�ʧ��\n"));
		return FALSE;
	} 

	// ����Server_PushMsg_db
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
			// �����ݿ��к��� ServerPushMsg_db ��
			int nRet= sqlite3_exec(pDB, lpszSelectServerPushMsg, ServerPushMsg_Callback, this, NULL);
		}
		if (SQLITE_OK == sqlite3_exec(pDB, lpszSelectClientPushMsg, NULL, NULL, &errmsg) )
		{
			// �����ݿ��к��� ClientPushMsg_db ��
			int nRet = sqlite3_exec(pDB, lpszSelectClientPushMsg, ClientPushMsg_Callback, this, NULL);
		}

	}
//////////////////////////////////////////////////////////////////////////
	sqlite3_exec(m_pTargetDB, "COMMIT;", NULL, NULL, &errmsg);
	 sqlite3_close(m_pTargetDB);
	return TRUE;
}
// �ϲ����ݿ��߳�
DWORD WINAPI CMergeDBDlg::_ThreadMergeDB(LPVOID lParam)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(lParam);
	DWORD dwRet = 0;
	pDlg->SearchDBFile(pDlg->m_strDBSrcRootPath.c_str() );
	pDlg->MergeDB();

	// ��ɣ�֪ͨ���߳�
	pDlg->PostMessage(WM_MERGE_DB_DONE);
	return dwRet;
}

// ɾ�����ݿ�����ͬ��¼�߳�
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

LRESULT CMergeDBDlg::OnMergeDBDone(WPARAM wParam, LPARAM lParam)					// ���ݿ�ϲ��Ѿ����
{
	m_bIsDBMerging = FALSE;					// �ϲ���־��Ϊ FALSE
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(TRUE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(TRUE);
	SetDlgItemText(IDB_MERGE_DB,TEXT("�ϲ����ݿ�"));
	return 1;
}
LRESULT CMergeDBDlg::OnDeleteSameRecDone(WPARAM wParam, LPARAM lParam)			// ɾ����ͬ��¼�Ѿ����
{
	GetDlgItem(IDB_MERGE_DB)->EnableWindow(TRUE);
	GetDlgItem(IDB_DELETE_SAME_REC)->EnableWindow(TRUE);
	SetDlgItemText(IDB_DELETE_SAME_REC,TEXT("ɾ����ͬ��¼"));

	return 1;
}

// �ļ�������Ի���ص�������ָ��ȱʡĿ¼
int CALLBACK CMergeDBDlg::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (BFFM_INITIALIZED == uMsg)
	{
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}

// Sqlite �ص� ,�������ļ�¼����Ŀ�����ݿ�� Server_PushMsg_db ����
int CMergeDBDlg::ServerPushMsg_Callback(void* pvoid, int argc, char** argv, char** col)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(pvoid);
	char szSql[256] = {0};
	sprintf_s(szSql, lpszInsertRec2Server_Push_db, argv[0], argv[1]);
	sqlite3_exec(pDlg->m_pTargetDB, szSql, NULL, NULL, NULL);

	return 0;
}

// Sqlite �ص� ,�������ļ�¼����Ŀ�����ݿ�� Client_PushMsg_db ����
int CMergeDBDlg::ClientPushMsg_Callback(void* pvoid, int argc, char** argv, char** col)
{
	CMergeDBDlg *pDlg = reinterpret_cast<CMergeDBDlg*>(pvoid);
	char szSql[256] = {0};
	sprintf_s(szSql, lpszInsertRec2Client_Push_db, argv[0], argv[1]);
	sqlite3_exec(pDlg->m_pTargetDB, szSql, NULL, NULL, NULL);

	return 0;
}