// PushServerStressTestDlg.cpp : ʵ���ļ�
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
// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
#pragma region ���ڶԻ���
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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

// CPushServerStressTestDlg �Ի���


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

	m_hEventPauseTest = CreateEvent(NULL, TRUE, TRUE, TEXT("��ͣ����"));
	m_hEventMinuteLock = CreateEvent(NULL, TRUE, TRUE, TEXT("ÿ�����¼�"));
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


// CPushServerStressTestDlg ��Ϣ�������

BOOL CPushServerStressTestDlg::OnInitDialog()
{
	
	CDialog::OnInitDialog();
	//������ʾ
	CenterWindow(GetDesktopWindow());
	
	//��������IP��ַĬ����Ϊ58.67.148.65���˿�Ϊ88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.12.130"));
	SetDlgItemInt(IDE_SER_PORT,88,FALSE);
	SetDlgItemInt(IDE_MSG_COUNT, 0);
	SetDlgItemInt(IDE_PUSHMSG_RATE, 1);

	// Tab
	//m_TabCtrl.InsertItem(0, _T("����һ"));
	//m_TabCtrl.InsertItem(1, _T("���Զ�"));
	//m_TabCtrl.InsertItem(2, _T("������"));
	//m_TabCtrl.InsertItem(4, _T("������"));
//	m_TabPage->Create(IDD_FORMVIEW, &m_TabCtrl);
	m_btnStopThread_I.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(FALSE);
	m_btn_EndStressTest.EnableWindow(FALSE);

	m_nTimeslot = 50;
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPushServerStressTestDlg::OnPaint()
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
HCURSOR CPushServerStressTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



// �����߳�һ

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
				// �ɹ�����chanllenge
				// �����������������
				TestSocket->DisConnect2PushSRV();
			}
			else
			{
				// ʧ�ܣ�д��Log
			}
		}
		delete TestSocket;

	}

	return 0;
}

// �����̶߳�
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
				// �ɹ�����chanllenge
			
				if (TestSocket->LoginServer())
				{
					// ע��������ɹ�
					TestSocket->DisConnect2PushSRV();
				} 
				else
				{
					// ע�������ʧ��
					// д��Log
				}
			
			}
			else
			{
				// GetChanllenge ʧ�ܣ�д��Log
				OutputDebugString(_T("GetChanllegeʧ��!\n"));
				TestSocket->DisConnect2PushSRV();
			}
		}
		delete TestSocket;

	}

	return 0;

}


// �����߳���
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
				// �ɹ�����chanllenge
			//	OutputDebugString(_T("Thread_III_Proc Complete Getchanllenge\n"));
				if (TestSocket->SendErrorCheckCode2PushSRV())
				{
					//OutputDebugString(_T("Thread_III_Proc Complete SendErrorCheckCode\n"));
					// ע��������ɹ�

					TestSocket->DisConnect2PushSRV();
					OutputDebugString(_T("�Ͽ�����\n\n"));
				//	OutputDebugString(_T("Thread_III_Proc Complete DisConnect2PushSRV\n"));
				} 
				else
				{
					// ע�������ʧ��
					// д��Log
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

// �����߳���
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
				/// �ɹ�����chanllenge
			//	OutputDebugString(_T("Thread_IV_Proc Complete Getchanllenge\n"));
				if (TestSocket->LoginServer())
				{
				//	OutputDebugString(_T("Thread_IV_Proc Complete SendErrorCheckCode\n"));
					// ע��������ɹ�
					if (TestSocket->SendPushMessage("dsfasfd", "afafdaf"))
					{
						TestSocket->DisConnect2PushSRV();
						OutputDebugString(_T("�Ͽ�����\n\n"));
					}
			//	OutputDebugString(_T("Thread_IV_Proc Complete DisConnect2PushSRV\n"));
				} 
				else
				{
					// ע�������ʧ��
					// д��Log
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

// ѭ����PushMessage�߳�
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
			/// �ɹ�����chanllenge
			OutputDebugString(_T("Thread_IV_Proc Complete Getchanllenge\n"));
			if (TestSocket->LoginServer())
			{
				// ��ʼ��ʱ
				pDlg->m_dwStartPushMsgTime = GetTickCount();
				pDlg->SetTimer(ID_TIMER_FRESH, 150, NULL);			// ��ʼOntimer
				ServerDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));
				while(pDlg->m_bIsThread_Test_Run)
				{
					TSTRING strPreTokenID = pDlg->m_strPreTokenID.GetBuffer();
					char szbuf[128] = {0};
					sprintf_s(szbuf,"%s%d", T2S(strPreTokenID).c_str(), 
								RandomNum(pDlg->m_nTokenID_StartNum, pDlg->m_nTokenIDNum));
					pDlg->m_strPreTokenID.ReleaseBuffer();
					// PushMessage����
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
//	// TODO: �ڴ���ӿؼ�֪ͨ����������
//	UpdateData(TRUE);
////	DWORD m_ServerIP;	
//	//m_ServerIPCtrl.GetAddress(m_ServerIP);
//	//SRVEndPoint.SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
//	//SRVEndPoint.Connect2PushSRV(m_dwServerIP,m_nServerPort);
////	SRVEndPoint.Connect2PushSRV("192.168.11.112", m_nServerPort);
//}



void CPushServerStressTestDlg::OnBnClickedStartThread()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_btnStartThread_I.EnableWindow(FALSE);
	m_btnStopThread_I.EnableWindow(TRUE);
	m_bIsThread_I_Run =TRUE;
	m_hThreadTest_I =  ::CreateThread(NULL, 0, Thread_I_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_I);
}

void CPushServerStressTestDlg::OnBnClickedStopThread()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_btnStartThread_I.EnableWindow(TRUE);
	m_btnStopThread_I.EnableWindow(FALSE);

	m_bIsThread_I_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_I, 3000);

	//DWORD dwExitCode = 0;
	//GetExitCodeThread(m_hThreadTest_I, &dwExitCode);

	//if (STILL_ACTIVE == dwExitCode)			//�߳��������У��ȴ��߳̽���
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_btnStartThread_II.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(TRUE);
	m_bIsThread_II_Run =TRUE;
	m_hThreadTest_II =  ::CreateThread(NULL, 0, Thread_II_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_II);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIi()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_btnStartThread_II.EnableWindow(TRUE);
	m_btnStopThread_II.EnableWindow(FALSE);

	m_bIsThread_II_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_II, 3000);
}

void CPushServerStressTestDlg::OnBnClickedStartThreadIii()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_btnStartThread_III.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(TRUE);
	m_bIsThread_III_Run =TRUE;
	m_hThreadTest_III =  ::CreateThread(NULL, 0, Thread_III_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_III);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIii()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_btnStartThread_III.EnableWindow(TRUE);
	m_btnStopThread_III.EnableWindow(FALSE);

	m_bIsThread_III_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_III, 3000);
}

void CPushServerStressTestDlg::OnBnClickedStartThreadIv()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_btnStartThread_IV.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(TRUE);
	m_bIsThread_IV_Run =TRUE;
	m_hThreadTest_IV =  ::CreateThread(NULL, 0, Thread_IV_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_IV);
}

void CPushServerStressTestDlg::OnBnClickedStopThreadIv()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_btnStartThread_IV.EnableWindow(TRUE);
	m_btnStopThread_IV.EnableWindow(FALSE);

	m_bIsThread_IV_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_IV, 3000);
}

void CPushServerStressTestDlg::OnBnClickedStressStartTest()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (UpdateData(TRUE))
	{
		m_btn_StartStressTest.EnableWindow(FALSE);
		m_btn_EndStressTest.EnableWindow(TRUE);
		m_bIsThread_Test_Run = TRUE;
		m_nCountPushMSg = 0;			// ����
		SetTimer(ID_TIMER_ONE_MIN, 60000, NULL);		// 2012-12-6 Add
	//	OnTimer(ID_TIMER_ONE_MIN);					// 2012-12-6 Add
		m_hThreadStressTest = ::CreateThread(NULL, 0, Thread_Stress_Test, this, 0, NULL);
	//	CloseHandle(m_hThreadStressTest);
	}


}

void CPushServerStressTestDlg::OnBnClickedStressStopTest()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_btn_StartStressTest.EnableWindow(TRUE);
	m_btn_EndStressTest.EnableWindow(FALSE);
	m_bIsThread_Test_Run = FALSE;
	KillTimer(ID_TIMER_ONE_MIN);
	SetEvent(m_hEventMinuteLock);
	SetEvent(m_hEventPauseTest);
	SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("��ͣ����"));
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventPauseTest, 0))
	{
		// ��ʱ���ڲ���
		ResetEvent(m_hEventPauseTest);
		SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("��������"));

	} 
	else if(WAIT_TIMEOUT == ::WaitForSingleObject(m_hEventPauseTest, 0))
	{
		// ��ʱ����ͣ����
		SetEvent(m_hEventPauseTest);
		SetDlgItemText(IDC_STRESS_PAUSE_TEST, _T("��ͣ����"));
	}

}
