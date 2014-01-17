// StressTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PushServerStressTest_Client.h"
#include "StressTestDlg.h"

// CStressTestDlg 对话框

IMPLEMENT_DYNAMIC(CStressTestDlg, CDialog)

CStressTestDlg::CStressTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStressTestDlg::IDD, pParent)
	, m_nThreadNum(0)
	, m_TokenID_StartNum(0)
	,m_bIsThreadRun(FALSE)
{
	m_hEventStopThread =  ::CreateEvent(NULL,FALSE,FALSE,NULL);

}

CStressTestDlg::~CStressTestDlg()
{
	CloseHandle(m_hEventStopThread);
}
BOOL CStressTestDlg::OnInitDialog()
{
	m_nThreadNum = 1;
	UpdateData(FALSE);
	m_btn_EndTest.EnableWindow(FALSE);
	return TRUE;
}

void CStressTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_THREADNUM, m_nThreadNum);
	DDV_MinMaxUInt(pDX, m_nThreadNum, 1, 1000);
	DDX_Text(pDX, IDC_EDIT_TOKENID_STARTNUM, m_TokenID_StartNum);
	DDX_Control(pDX, IDC_BTN_START_TEST, m_btn_StartTest);
	DDX_Control(pDX, IDC_BTN_END_TEST, m_btn_EndTest);
	DDX_Text(pDX, IDC_EDIT_TOKENID_PRE, m_strPreTokenID);
}


BEGIN_MESSAGE_MAP(CStressTestDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_START_TEST, &CStressTestDlg::OnBnClickedBtnStartTest)
	ON_BN_CLICKED(IDC_BTN_END_TEST, &CStressTestDlg::OnBnClickedBtnEndTest)
END_MESSAGE_MAP()


// CStressTestDlg 消息处理程序

// 点击开始按钮
void CStressTestDlg::OnBnClickedBtnStartTest()
{
	UpdateData(TRUE);
	m_btn_StartTest.EnableWindow(FALSE);
	m_btn_EndTest.EnableWindow(TRUE);
	m_bIsThreadRun =TRUE;
	ClientDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));
	m_hThread = ::CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
	CloseHandle(m_hThread);
}

// 点击停止按钮
void CStressTestDlg::OnBnClickedBtnEndTest()
{
	m_btn_StartTest.EnableWindow(TRUE);
	m_btn_EndTest.EnableWindow(FALSE);
	m_bIsThreadRun = FALSE;
	//WaitForSingleObject(m_hThread, 4000);
		
	// 激活停止线程事件，让ThreadProc执行停止操作
	SetEvent(m_hEventStopThread);
	//WaitForSingleObject(m_hThread, INFINITE);
	ClientDB::GetInstance()->ExecuteSQL(TEXT("COMMIT;"));
}

DWORD WINAPI CStressTestDlg::ThreadProc(LPVOID lParam)
{
	CStressTestDlg* pDlg = reinterpret_cast<CStressTestDlg*>(lParam);
	CClientEndPoint *ClientArr = new CClientEndPoint[pDlg->m_nThreadNum];

	DWORD dwServerIP =0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_IP, 0, reinterpret_cast<LPARAM>(&dwServerIP));
	UINT nServerPort = 0;
	pDlg->GetParent()->SendMessage(WM_GET_PUSHSRV_PORT, 0, reinterpret_cast<LPARAM>(&nServerPort));
	
	for (int i=0; i<pDlg->m_nThreadNum; ++i)
	{
		 TSTRING strPreTonkenID = pDlg->m_strPreTokenID.GetBuffer();
		char szbuf[128] = {0};
		sprintf_s(szbuf, "%s%d",T2S(strPreTonkenID).c_str(), pDlg->m_TokenID_StartNum+i);
		ClientArr[i].SetTokenID(szbuf);
		pDlg->m_strPreTokenID.ReleaseBuffer();
		ClientArr[i].SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		ClientArr[i].SetPushSRVAddr(dwServerIP, nServerPort);
	}

	for (int i=0; i<pDlg->m_nThreadNum;++i)
	{
		//Sleep(100);
		ClientArr[i].StartRecvDataTest();
	}

	WaitForSingleObject(pDlg->m_hEventStopThread, INFINITE);
	for (int i =0; i<pDlg->m_nThreadNum; ++i)
	{
		ClientArr[i].StopRecvDataTest();

	}
	delete [] ClientArr;

	return 0;
	
}