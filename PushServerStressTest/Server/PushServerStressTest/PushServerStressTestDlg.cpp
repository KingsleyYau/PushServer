// PushServerStressTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PushServerStressTest.h"
#include "PushServerStressTestDlg.h"
#include <AtlConv.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ID_TIMER_ONE_MIN 1021
#define ID_TIMER_FRESH 1011

int RandomNum(int nMin, int nRange)
{

	int randNum = (((double) rand() / 
		(double) RAND_MAX) * (nRange-1) + nMin);
	return randNum;
}

string RandomStr()
{
	char szbuf[10] = {0};
	for (int i = 0; i<9;++i)
	{
		szbuf[i] = rand() %26+97;
	}
	string str = szbuf;
	return str;
}
TSTRING FormatOutputTime(DWORD dwMilliSec)
{
	ASSERT(dwMilliSec < (24*3600*1000) );
	UINT nHour = 0;
	UINT nMin = 0;
	UINT nSec = 0;
	UINT nTotalSec= dwMilliSec/1000;
	nHour = nTotalSec/3600;
	nTotalSec = nTotalSec%3600;
	nMin = nTotalSec /60;
	nSec = nTotalSec%60;

	TCHAR szTime[12] = {0};
	_stprintf_s(szTime, 12, _T("%02d:%02d:%02d"), nHour, nMin, nSec);
	TSTRING strOutput(szTime);

	return strOutput;
}
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
	, m_bIsThread_Test_Run(FALSE)
	, m_strPreTokenID(_T(""))
	, m_nTokenID_StartNum(0)
	, m_nTokenIDNum(1)
	, m_nCountPushMSg(0)
	, m_hEventPauseTest(NULL)
	, m_hEventMinuteLock(NULL)
	, m_nPushMsgRateNum(0)
	, m_nCountPushMsgPerMin(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_hEventPauseTest = CreateEvent(NULL, TRUE, TRUE, TEXT("暂停测试"));
	m_hEventMinuteLock = CreateEvent(NULL, TRUE, TRUE, TEXT("每分钟事件"));
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
	CloseHandle(m_hEventPauseTest);
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
	DDX_Control(pDX, IDC_STRESS_START_TEST, m_btn_StartStressTest);
	DDX_Control(pDX, IDC_STRESS_STOP_TEST, m_btn_EndStressTest);
	DDX_Text(pDX, IDC_EDIT_TOKENID_PRE, m_strPreTokenID);
	DDX_Text(pDX, IDC_EDIT_TOKENID_STARTNUM, m_nTokenID_StartNum);
	DDX_Text(pDX, IDC_TOKENID_NUM, m_nTokenIDNum);
	DDX_Control(pDX, IDE_SPEND_TIME, m_SpendTime);
	DDX_Control(pDX, IDE_MSG_COUNT, m_edit_MsgCount);
	DDX_Text(pDX, IDE_PUSHMSG_RATE, m_nPushMsgRateNum);
	DDV_MinMaxUInt(pDX, m_nPushMsgRateNum, 1, 50000);
}

BEGIN_MESSAGE_MAP(CPushServerStressTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
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
	ON_BN_CLICKED(IDC_STRESS_START_TEST, &CPushServerStressTestDlg::OnBnClickedStressStartTest)
	ON_BN_CLICKED(IDC_STRESS_STOP_TEST, &CPushServerStressTestDlg::OnBnClickedStressStopTest)
	ON_BN_CLICKED(IDC_STRESS_PAUSE_TEST, &CPushServerStressTestDlg::OnBnClickedStressPauseTest)
END_MESSAGE_MAP()


// CPushServerStressTestDlg 消息处理程序

BOOL CPushServerStressTestDlg::OnInitDialog()
{
	
	CDialog::OnInitDialog();
	//居中显示
	CenterWindow(GetDesktopWindow());
	
	//将服务器IP地址默认设为58.67.148.65，端口为88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.12.130"));
	SetDlgItemInt(IDE_SER_PORT,88,FALSE);
	SetDlgItemInt(IDE_MSG_COUNT, 0);
	SetDlgItemInt(IDE_PUSHMSG_RATE, 1);

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
	m_btn_EndStressTest.EnableWindow(FALSE);

	m_nTimeslot = 50;
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
					if (TestSocket->SendPushMessage("dsfasfd", "afafdaf"))
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

// 循环发PushMessage线程
DWORD WINAPI CPushServerStressTestDlg::Thread_Stress_Test(LPVOID lParam)
{
	CPushServerStressTestDlg *pDlg = reinterpret_cast<CPushServerStressTestDlg*>(lParam);
	srand(GetTickCount());
	CServerEndPoint *TestSocket = new CServerEndPoint;
	TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");


	if (TestSocket->Connect2PushSRV(pDlg->m_dwServerIP, pDlg->m_nServerPort))
	{
		TestSocket->SetTimeOut(4000);
		OutputDebugString(_T("Thread_IV_Proc Complete Connect 2 Push Server\n"));
		if (TestSocket->GetChallenge())
		{
			/// 成功返回chanllenge
			OutputDebugString(_T("Thread_IV_Proc Complete Getchanllenge\n"));
			if (TestSocket->LoginServer())
			{
				// 开始计时
				pDlg->m_dwStartPushMsgTime = GetTickCount();
				pDlg->SetTimer(ID_TIMER_FRESH, 150, NULL);			// 开始Ontimer
				ServerDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));
				while(pDlg->m_bIsThread_Test_Run)
				{
					TSTRING strPreTokenID = pDlg->m_strPreTokenID.GetBuffer();
					char szbuf[128] = {0};
					sprintf_s(szbuf,"%s%d", T2S(strPreTokenID).c_str(), 
								RandomNum(pDlg->m_nTokenID_StartNum, pDlg->m_nTokenIDNum));
					pDlg->m_strPreTokenID.ReleaseBuffer();
					// PushMessage计数
					if (TestSocket->SendPushMessage(szbuf, RandomStr()))
					{
						++pDlg->m_nCountPushMSg;
						InterlockedIncrement(&pDlg->m_nCountPushMsgPerMin);

					}
					pDlg->m_csAtomicOperation.Lock();
					if ( 0==(pDlg->m_nCountPushMsgPerMin % pDlg->m_nPushMsgRateNum))
					{
						ResetEvent(pDlg->m_hEventMinuteLock);
					}
					pDlg->m_csAtomicOperation.Unlock();
					WaitForSingleObject(pDlg->m_hEventMinuteLock, INFINITE);
					WaitForSingleObject(pDlg->m_hEventPauseTest, INFINITE);
				
				}

				ServerDB::GetInstance()->ExecuteSQL(TEXT("COMMIT;"));
			}
		}
	}
	delete TestSocket;
	TestSocket = NULL;
	return FALSE;
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

void CPushServerStressTestDlg::OnBnClickedStressStartTest()
{
	// TODO: 在此添加控件通知处理程序代码
	if (UpdateData(TRUE))
	{
		m_btn_StartStressTest.EnableWindow(FALSE);
		m_btn_EndStressTest.EnableWindow(TRUE);
		m_bIsThread_Test_Run = TRUE;
		m_nCountPushMSg = 0;			// 清零
		SetTimer(ID_TIMER_ONE_MIN, 60000, NULL);		// 2012-12-6 Add
	//	OnTimer(ID_TIMER_ONE_MIN);					// 2012-12-6 Add
		m_hThreadStressTest = ::CreateThread(NULL, 0, Thread_Stress_Test, this, 0, NULL);
	//	CloseHandle(m_hThreadStressTest);
	}


}

void CPushServerStressTestDlg::OnBnClickedStressStopTest()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btn_StartStressTest.EnableWindow(TRUE);
	m_btn_EndStressTest.EnableWindow(FALSE);
	m_bIsThread_Test_Run = FALSE;
	KillTimer(ID_TIMER_ONE_MIN);
	SetEvent(m_hEventMinuteLock);
	SetEvent(m_hEventPauseTest);
	SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("暂停测试"));
	WaitForSingleObject(m_hThreadStressTest,INFINITE);
	KillTimer(ID_TIMER_FRESH);
	OnTimer(ID_TIMER_FRESH);

}
void CPushServerStressTestDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (ID_TIMER_ONE_MIN == nIDEvent)
	{
		m_csAtomicOperation.Lock();
		InterlockedExchange(&m_nCountPushMsgPerMin, 0);
		SetEvent(m_hEventMinuteLock);
		m_csAtomicOperation.Unlock();
	} 
	else if (ID_TIMER_FRESH == nIDEvent)
	{
		DWORD dwElapsedTime = GetTickCount() - m_dwStartPushMsgTime;
		TSTRING strElapsedTime = FormatOutputTime(dwElapsedTime);
		SetDlgItemText(IDE_SPEND_TIME, strElapsedTime.c_str());
		SetDlgItemInt(IDE_MSG_COUNT, m_nCountPushMSg);
	}

	CDialog::OnTimer(nIDEvent);
}
void CPushServerStressTestDlg::OnBnClickedStressPauseTest()
{
	// TODO: 在此添加控件通知处理程序代码
	if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventPauseTest, 0))
	{
		// 此时正在测试
		ResetEvent(m_hEventPauseTest);
		SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("继续测试"));

	} 
	else if(WAIT_TIMEOUT == ::WaitForSingleObject(m_hEventPauseTest, 0))
	{
		// 此时正暂停测试
		SetEvent(m_hEventPauseTest);
		SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("暂停测试"));
	}

}
