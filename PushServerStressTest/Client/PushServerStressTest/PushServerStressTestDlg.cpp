// PushServerStressTestDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PushServerStressTest.h"
#include "PushServerStressTestDlg.h"
#include <AtlConv.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CPushServerStressTestDlg ��Ϣ�������

BOOL CPushServerStressTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//������ʾ
	CenterWindow(GetDesktopWindow());
	
	//��������IP��ַĬ����Ϊ58.67.148.65���˿�Ϊ88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.11.112"));
	SetDlgItemInt(IDE_SER_PORT,83,FALSE);

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

	m_nTimeslot = 100;
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
					if (TestSocket->SendPushMessage(_T("dsfasfd"), _T("afafdaf")))
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
