// PushServerStressTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PushServerStressTest.h"
#include "PushServerStressTestDlg.h"
#include <AtlConv.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
#pragma region 关于对话框
class CAboutDlg : public CDialog
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
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()
#pragma endregion

// CPushServerStressTestDlg 对话框




CPushServerStressTestDlg::CPushServerStressTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPushServerStressTestDlg::IDD, pParent)
	, m_nServerPort(0)
	, m_dwServerIP(0)
	, m_bIsThread_I_Run(FALSE)
	, m_bIsThread_II_Run(FALSE)
	, m_bIsThread_III_Run(FALSE)
	, m_bIsThread_IV_Run(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//Initialize WinSock
	WORD wVersionRequested;
	WSADATA wsaData;
	int nErr;

	wVersionRequested = MAKEWORD( 2, 2 );
	nErr = WSAStartup( wVersionRequested, &wsaData );

	if (nErr !=0 )
	{
		OutputDebugString(_T("Winsock Initialize faild !"));
		return;
	}
	if (LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2)
	{
		OutputDebugString(_T("Winsock Version incorret!"));
		WSACleanup();
		return;
	}
}
CPushServerStressTestDlg::~CPushServerStressTestDlg()
{
	WSACleanup();
}

void CPushServerStressTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDE_SEND_CONTENT, m_SendContent);
	DDX_IPAddress(pDX, IDC_SER_IPADDR, m_dwServerIP);
	DDX_Text(pDX, IDE_SER_PORT, m_nServerPort);
	DDV_MinMaxUInt(pDX, m_nServerPort, 0, 65535);
	//DDX_Control(pDX, IDC_TAB1, m_TabCtrl);
	DDX_Control(pDX, IDB_START_THREAD_I, m_btnStartThread_I);
	DDX_Control(pDX, IDB_STOP_THREAD_I, m_btnStopThread_I);
	DDX_Control(pDX, IDB_START_THREAD_II, m_btnStartThread_II);
	DDX_Control(pDX, IDB_STOP_THREAD_II, m_btnStopThread_II);
	DDX_Control(pDX, IDB_START_THREAD_III, m_btnStartThread_III);
	DDX_Control(pDX, IDB_STOP_THREAD_III, m_btnStopThread_III);
	DDX_Control(pDX, IDB_START_THREAD_IV, m_btnStartThread_IV);
	DDX_Control(pDX, IDB_STOP_THREAD_IV, m_btnStopThread_IV);
}

BEGIN_MESSAGE_MAP(CPushServerStressTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	//ON_BN_CLICKED(IDB_CONN, &CPushServerStressTestDlg::OnBnClickedConn)
	ON_BN_CLICKED(IDB_START_THREAD_I, &CPushServerStressTestDlg::OnBnClickedStartThread)
	ON_BN_CLICKED(IDB_STOP_THREAD_I, &CPushServerStressTestDlg::OnBnClickedStopThread)

	ON_BN_CLICKED(IDB_START_THREAD_II, &CPushServerStressTestDlg::OnBnClickedStartThreadIi)
	ON_BN_CLICKED(IDB_STOP_THREAD_II, &CPushServerStressTestDlg::OnBnClickedStopThreadIi)
	ON_BN_CLICKED(IDB_START_THREAD_III, &CPushServerStressTestDlg::OnBnClickedStartThreadIii)
	ON_BN_CLICKED(IDB_STOP_THREAD_III, &CPushServerStressTestDlg::OnBnClickedStopThreadIii)
	ON_BN_CLICKED(IDB_START_THREAD_IV, &CPushServerStressTestDlg::OnBnClickedStartThreadIv)
	ON_BN_CLICKED(IDB_STOP_THREAD_IV, &CPushServerStressTestDlg::OnBnClickedStopThreadIv)
END_MESSAGE_MAP()


// CPushServerStressTestDlg 消息处理程序

BOOL CPushServerStressTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//居中显示
	CenterWindow(GetDesktopWindow());
	
	//将服务器IP地址默认设为58.67.148.65，端口为88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.11.112"));
	SetDlgItemInt(IDE_SER_PORT,83,FALSE);

	// Tab
	//m_TabCtrl.InsertItem(0, _T("测试一"));
	//m_TabCtrl.InsertItem(1, _T("测试二"));
	//m_TabCtrl.InsertItem(2, _T("测试三"));
	//m_TabCtrl.InsertItem(4, _T("测试四"));
//	m_TabPage->Create(IDD_FORMVIEW, &m_TabCtrl);
	m_btnStopThread_I.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(FALSE);

	m_nTimeslot = 100;
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

void CPushServerStressTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPushServerStressTestDlg::OnPaint()
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
HCURSOR CPushServerStressTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



// 测试线程一

DWORD WINAPI CPushServerStressTestDlg::Thread_I_Proc(LPVOID lParam)
{
	CPushServerStressTestDlg *pDlg = reinterpret_cast<CPushServerStressTestDlg*>(lParam);

	while(pDlg->m_bIsThread_I_Run)
	{
		Sleep(pDlg->m_nTimeslot);
		CServerEndPoint *TestSocket = new CServerEndPoint;
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(pDlg->m_dwServerIP, pDlg->m_nServerPort))
		{
			if (TestSocket->GetChallenge())
			{
				// 成功返回chanllenge
				// 结束与服务器的连接
				TestSocket->DisConnect2PushSRV();
			}
			else
			{
				// 失败，写入Log
			}
		}
		delete TestSocket;

	}

	return 0;
}

// 测试线程二
DWORD WINAPI CPushServerStressTestDlg::Thread_II_Proc(LPVOID lParam)
{
	CPushServerStressTestDlg *pDlg = reinterpret_cast<CPushServerStressTestDlg*>(lParam);

	while(pDlg->m_bIsThread_II_Run)
	{
		Sleep(pDlg->m_nTimeslot);
		CServerEndPoint *TestSocket = new CServerEndPoint;
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(pDlg->m_dwServerIP, pDlg->m_nServerPort))
		{
			if (TestSocket->GetChallenge())
			{
				// 成功返回chanllenge
			
				if (TestSocket->LoginServer())
				{
					// 注册服务器成功
					TestSocket->DisConnect2PushSRV();
				} 
				else
				{
					// 注册服务器失败
					// 写入Log
				}
			
			}
			else
			{
				// GetChanllenge 失败，写入Log
				OutputDebugString(_T("GetChanllege失败!\n"));
				TestSocket->DisConnect2PushSRV();
			}
		}
		delete TestSocket;

	}

	return 0;

}


// 测试线程三
DWORD WINAPI CPushServerStressTestDlg::Thread_III_Proc(LPVOID lParam)
{
	CPushServerStressTestDlg *pDlg = reinterpret_cast<CPushServerStressTestDlg*>(lParam);

	while(pDlg->m_bIsThread_III_Run)
	{
	
		Sleep(pDlg->m_nTimeslot);
		CServerEndPoint *TestSocket = new CServerEndPoint;
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(pDlg->m_dwServerIP, pDlg->m_nServerPort))
		{
			//OutputDebugString(_T("Thread_III_Proc Complete Connect 2 Push Server\n"));
			if (TestSocket->GetChallenge())
			{
				// 成功返回chanllenge
			//	OutputDebugString(_T("Thread_III_Proc Complete Getchanllenge\n"));
				if (TestSocket->SendErrorCheckCode2PushSRV())
				{
					//OutputDebugString(_T("Thread_III_Proc Complete SendErrorCheckCode\n"));
					// 注册服务器成功

					TestSocket->DisConnect2PushSRV();
					OutputDebugString(_T("断开连接\n\n"));
				//	OutputDebugString(_T("Thread_III_Proc Complete DisConnect2PushSRV\n"));
				} 
				else
				{
					// 注册服务器失败
					// 写入Log
				}
			}
			else
			{
				TestSocket->DisConnect2PushSRV();
			}
		}
		delete TestSocket;
	//	OutputDebugString(_T("Thread_III_Proc delete Socket\n"));

	}

	return 0;

}

// 测试线程四
DWORD WINAPI CPushServerStressTestDlg::Thread_IV_Proc(LPVOID lParam)
{
	CPushServerStressTestDlg *pDlg = reinterpret_cast<CPushServerStressTestDlg*>(lParam);

	while(pDlg->m_bIsThread_IV_Run)
	{
	//	OutputDebugString(_T("Thread_IV_Proc Run\n"));
		Sleep(pDlg->m_nTimeslot);
		CServerEndPoint *TestSocket = new CServerEndPoint;
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(pDlg->m_dwServerIP, pDlg->m_nServerPort))
		{
			//OutputDebugString(_T("Thread_IV_Proc Complete Connect 2 Push Server\n"));
			if (TestSocket->GetChallenge())
			{
				/// 成功返回chanllenge
			//	OutputDebugString(_T("Thread_IV_Proc Complete Getchanllenge\n"));
				if (TestSocket->LoginServer())
				{
				//	OutputDebugString(_T("Thread_IV_Proc Complete SendErrorCheckCode\n"));
					// 注册服务器成功
					if (TestSocket->SendPushMessage(_T("dsfasfd"), _T("afafdaf")))
					{
						TestSocket->DisConnect2PushSRV();
						OutputDebugString(_T("断开连接\n\n"));
					}
			//	OutputDebugString(_T("Thread_IV_Proc Complete DisConnect2PushSRV\n"));
				} 
				else
				{
					// 注册服务器失败
					// 写入Log
				}
			}
			else
			{
				TestSocket->DisConnect2PushSRV();
			}
		}
		delete TestSocket;
	//	OutputDebugString(_T("Thread_IV_Proc delete Socket\n"));

	}

	return 0;

}

//void CPushServerStressTestDlg::OnBnClickedConn()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	UpdateData(TRUE);
////	DWORD m_ServerIP;	
//	//m_ServerIPCtrl.GetAddress(m_ServerIP);
//	//SRVEndPoint.SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
//	//SRVEndPoint.Connect2PushSRV(m_dwServerIP,m_nServerPort);
////	SRVEndPoint.Connect2PushSRV("192.168.11.112", m_nServerPort);
//}



void CPushServerStressTestDlg::OnBnClickedStartThread()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_I.EnableWindow(FALSE);
	m_btnStopThread_I.EnableWindow(TRUE);
	m_bIsThread_I_Run =TRUE;
	m_hThreadTest_I =  ::CreateThread(NULL, 0, Thread_I_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_I);
}

void CPushServerStressTestDlg::OnBnClickedStopThread()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_I.EnableWindow(TRUE);
	m_btnStopThread_I.EnableWindow(FALSE);

	m_bIsThread_I_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_I, 3000);

	//DWORD dwExitCode = 0;
	//GetExitCodeThread(m_hThreadTest_I, &dwExitCode);

	//if (STILL_ACTIVE == dwExitCode)			//线程正在运行，等待线程结束
	//{
	//	while(TRUE)
	//	{
	//		MSG uMsg;
	//		DWORD dwRetCode =MsgWaitForMultipleObjects(1,&m_hThread,FALSE,INFINITE,QS_ALLINPUT);
	//		if (dwRetCode == (WAIT_OBJECT_0))
	//		{
	//			break;
	//		} 
	//		else 
	//		{ 
	//			PeekMessage(&uMsg, m_hWndListView, 0, 0, PM_REMOVE);
	//			DispatchMessage(&uMsg); 
	//		}
	//	}

	//}

}



void CPushServerStressTestDlg::OnBnClickedStartThreadIi()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_II.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(TRUE);
	m_bIsThread_II_Run =TRUE;
	m_hThreadTest_II =  ::CreateThread(NULL, 0, Thread_II_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_II);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIi()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_II.EnableWindow(TRUE);
	m_btnStopThread_II.EnableWindow(FALSE);

	m_bIsThread_II_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_II, 3000);
}

void CPushServerStressTestDlg::OnBnClickedStartThreadIii()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_III.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(TRUE);
	m_bIsThread_III_Run =TRUE;
	m_hThreadTest_III =  ::CreateThread(NULL, 0, Thread_III_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_III);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIii()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_III.EnableWindow(TRUE);
	m_btnStopThread_III.EnableWindow(FALSE);

	m_bIsThread_III_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_III, 3000);
}

void CPushServerStressTestDlg::OnBnClickedStartThreadIv()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_IV.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(TRUE);
	m_bIsThread_IV_Run =TRUE;
	m_hThreadTest_IV =  ::CreateThread(NULL, 0, Thread_IV_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_IV);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIv()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_IV.EnableWindow(TRUE);
	m_btnStopThread_IV.EnableWindow(FALSE);

	m_bIsThread_IV_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_IV, 3000);
}
