// RouTineTest.cpp : 实现文件
//

#include "stdafx.h"
#include "PushServerStressTest_Client.h"
#include "RouTineTest.h"


// CRouTineTest 对话框

IMPLEMENT_DYNAMIC(CRouTineTest, CDialog)

CRouTineTest::CRouTineTest(CWnd* pParent /*=NULL*/)
: CDialog(CRouTineTest::IDD, pParent)
	,m_bIsThread_I_Run(FALSE)
	,m_bIsThread_II_Run(FALSE)
	,m_bIsThread_III_Run(FALSE)
	,m_bIsThread_IV_Run(FALSE)
	,m_nTimeslot(300)
{

}
CRouTineTest::~CRouTineTest()
{

}
BOOL CRouTineTest::OnInitDialog()
{
	UpdateData(FALSE);
	m_btnStopThread_I.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(FALSE);
	
	return TRUE;
}

void CRouTineTest::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDB_START_THREAD_I, m_btnStartThread_I);
	DDX_Control(pDX, IDB_STOP_THREAD_I, m_btnStopThread_I);
	DDX_Control(pDX, IDB_START_THREAD_II, m_btnStartThread_II);
	DDX_Control(pDX, IDB_STOP_THREAD_II, m_btnStopThread_II);
	DDX_Control(pDX, IDB_START_THREAD_III, m_btnStartThread_III);
	DDX_Control(pDX, IDB_STOP_THREAD_III, m_btnStopThread_III);
	DDX_Control(pDX, IDB_START_THREAD_IV, m_btnStartThread_IV);
	DDX_Control(pDX, IDB_STOP_THREAD_IV, m_btnStopThread_IV);
}


BEGIN_MESSAGE_MAP(CRouTineTest, CDialog)
	ON_BN_CLICKED(IDB_START_THREAD_I, &CRouTineTest::OnBnClickedStartThreadI)
	ON_BN_CLICKED(IDB_STOP_THREAD_I, &CRouTineTest::OnBnClickedStopThreadI)
	ON_BN_CLICKED(IDB_START_THREAD_II, &CRouTineTest::OnBnClickedStartThreadIi)
	ON_BN_CLICKED(IDB_STOP_THREAD_II, &CRouTineTest::OnBnClickedStopThreadIi)
	ON_BN_CLICKED(IDB_START_THREAD_III, &CRouTineTest::OnBnClickedStartThreadIii)
	ON_BN_CLICKED(IDB_STOP_THREAD_III, &CRouTineTest::OnBnClickedStopThreadIii)
	ON_BN_CLICKED(IDB_START_THREAD_IV, &CRouTineTest::OnBnClickedStartThreadIv)
	ON_BN_CLICKED(IDB_STOP_THREAD_IV, &CRouTineTest::OnBnClickedStopThreadIv)
END_MESSAGE_MAP()


// CRouTineTest 消息处理程序

void CRouTineTest::OnBnClickedStartThreadI()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_I.EnableWindow(FALSE);
	m_btnStopThread_I.EnableWindow(TRUE);
	m_bIsThread_I_Run =TRUE;
	m_hThreadTest_I =  ::CreateThread(NULL, 0, Thread_I_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_I);

}

void CRouTineTest::OnBnClickedStopThreadI()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_I.EnableWindow(TRUE);
	m_btnStopThread_I.EnableWindow(FALSE);

	m_bIsThread_I_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_I, 3000);
}

void CRouTineTest::OnBnClickedStartThreadIi()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_II.EnableWindow(FALSE);
	m_btnStopThread_II.EnableWindow(TRUE);
	m_bIsThread_II_Run =TRUE;
	m_hThreadTest_II =  ::CreateThread(NULL, 0, Thread_II_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_II);
}

void CRouTineTest::OnBnClickedStopThreadIi()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_II.EnableWindow(TRUE);
	m_btnStopThread_II.EnableWindow(FALSE);
	m_bIsThread_II_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_II, 3000);
}

void CRouTineTest::OnBnClickedStartThreadIii()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_III.EnableWindow(FALSE);
	m_btnStopThread_III.EnableWindow(TRUE);
	m_bIsThread_III_Run =TRUE;
	m_hThreadTest_III =  ::CreateThread(NULL, 0, Thread_III_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_III);
}

void CRouTineTest::OnBnClickedStopThreadIii()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_III.EnableWindow(TRUE);
	m_btnStopThread_III.EnableWindow(FALSE);
	m_bIsThread_III_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_III, 3000);
}

void CRouTineTest::OnBnClickedStartThreadIv()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_btnStartThread_IV.EnableWindow(FALSE);
	m_btnStopThread_IV.EnableWindow(TRUE);
	m_bIsThread_IV_Run =TRUE;
	m_hThreadTest_IV =  ::CreateThread(NULL, 0, Thread_IV_Proc, this, 0, NULL);
	CloseHandle(m_hThreadTest_IV);
}

void CRouTineTest::OnBnClickedStopThreadIv()
{
	// TODO: 在此添加控件通知处理程序代码
	m_btnStartThread_IV.EnableWindow(TRUE);
	m_btnStopThread_IV.EnableWindow(FALSE);
	m_bIsThread_IV_Run = FALSE;
	WaitForSingleObject(m_hThreadTest_IV, 3000);
}





 //测试线程一
DWORD WINAPI CRouTineTest::Thread_I_Proc(LPVOID lParam)
{
	CRouTineTest *pDlg = reinterpret_cast<CRouTineTest*>(lParam);
	DWORD dwServerIP =0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_IP, 0, reinterpret_cast<LPARAM>(&dwServerIP));
	UINT nServerPort = 0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_PORT, 0, reinterpret_cast<LPARAM>(&nServerPort));

	while(pDlg->m_bIsThread_I_Run)
	{
		Sleep(pDlg->m_nTimeslot);
		CClientEndPoint*TestSocket = new CClientEndPoint;
		TestSocket->SetTokenID("adfwenl234lkjasdgpojasdrfwe");
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(dwServerIP, nServerPort))
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
DWORD WINAPI CRouTineTest::Thread_II_Proc(LPVOID lParam)
{
	static UINT nTotal = 0;
	CRouTineTest *pDlg = reinterpret_cast<CRouTineTest*>(lParam);
	DWORD dwServerIP =0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_IP, 0, reinterpret_cast<LPARAM>(&dwServerIP));
	UINT nServerPort = 0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_PORT, 0, reinterpret_cast<LPARAM>(&nServerPort));

	while(pDlg->m_bIsThread_II_Run)
	{
		Sleep(pDlg->m_nTimeslot);
		CClientEndPoint *TestSocket = new CClientEndPoint; 
		TestSocket->SetTokenID("adfwenl234lkjasdgpojasdrfwe");
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(dwServerIP, nServerPort))
		{
			if (TestSocket->GetChallenge())
			{
				// 成功返回chanllenge
				if (TestSocket->RegisterTokenID())
				{
					// 注册服务器成功
					TestSocket->DisConnect2PushSRV();
					TCHAR szbuf[24] = {0};
					_stprintf(szbuf,_T("断开连接%d\n\n"),nTotal);
					OutputDebugString(szbuf);
					++nTotal;
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
DWORD WINAPI CRouTineTest::Thread_III_Proc(LPVOID lParam)
{
	static UINT nTotal = 0;
	CRouTineTest *pDlg = reinterpret_cast<CRouTineTest*>(lParam);
	DWORD dwServerIP =0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_IP, 0, reinterpret_cast<LPARAM>(&dwServerIP));
	UINT nServerPort = 0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_PORT, 0, reinterpret_cast<LPARAM>(&nServerPort));

	while(pDlg->m_bIsThread_III_Run)
	{
		Sleep(pDlg->m_nTimeslot);
		CClientEndPoint *TestSocket = new CClientEndPoint; 
		TestSocket->SetTokenID("adfwenl234lkjasdgpojasdrfwe");
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(dwServerIP, nServerPort))
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
					TCHAR szbuf[24] = {0};
					_stprintf(szbuf,_T("断开连接%d\n\n"),nTotal);
					OutputDebugString(szbuf);
					++nTotal;
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
DWORD WINAPI CRouTineTest::Thread_IV_Proc(LPVOID lParam)
{
	static UINT nTotal = 0;
	CRouTineTest *pDlg = reinterpret_cast<CRouTineTest*>(lParam);
	DWORD dwServerIP =0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_IP, 0, reinterpret_cast<LPARAM>(&dwServerIP));
	UINT nServerPort = 0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_PORT, 0, reinterpret_cast<LPARAM>(&nServerPort));

	while(pDlg->m_bIsThread_IV_Run)
	{
		//	OutputDebugString(_T("Thread_IV_Proc Run\n"));
		Sleep(pDlg->m_nTimeslot);
		CClientEndPoint *TestSocket = new CClientEndPoint; 
		TestSocket->SetTokenID("adfwenl234lkjasdgpojasdrfwe");
		TestSocket->SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		if (TestSocket->Connect2PushSRV(dwServerIP, nServerPort))
		{
			//OutputDebugString(_T("Thread_IV_Proc Complete Connect 2 Push Server\n"));
			if (TestSocket->GetChallenge())
			{
				/// 成功返回chanllenge
				//	OutputDebugString(_T("Thread_IV_Proc Complete Getchanllenge\n"));
				if (TestSocket->RegisterTokenID())
				{
					//	OutputDebugString(_T("Thread_IV_Proc Complete SendErrorCheckCode\n"));
					// 注册服务器成功
					if (TestSocket->GetPushMessage())
					{
						TestSocket->DisConnect2PushSRV();
			
						TCHAR szbuf[24] = {0};
						_stprintf(szbuf,_T("断开连接%d\n\n"),nTotal);
						OutputDebugString(szbuf);
						++nTotal;
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

