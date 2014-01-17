// IOCP_ClientDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "IOCP_Client.h"
#include "IOCP_ClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  ID_TIMER_PER_SEC 1011
#define  ID_TIMER_FRESH_INTERVAL 1012

// CIOCP_ClientDlg �Ի���

CIOCP_ClientDlg::CIOCP_ClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIOCP_ClientDlg::IDD, pParent)
	, m_nClientNum(1)
	, m_nServerPort(0)
	, m_strPreTokenID(_T(""))
	, m_dwServerIP(0)
	, m_TokenID_StartNum(0)
	, m_nMinOnlineTime(0)
	, m_nMaxOnlineTime(0)
	, m_nMaxOfflineTime(0)
	, m_nMinOfflineTime(0)
	, m_nTestSpendTime(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIOCP_ClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CONCURRENT_NUM, m_nClientNum);
	DDV_MinMaxUInt(pDX, m_nClientNum, 1, 30000);
	DDX_Text(pDX, IDE_SERVER_PORT, m_nServerPort);
	DDV_MinMaxUInt(pDX, m_nServerPort, 0, 65535);
	DDX_Text(pDX, IDC_PRE_TOKENID, m_strPreTokenID);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERVERIP, m_dwServerIP);
	DDX_Text(pDX, IDC_TOKENID_STARTNUM, m_TokenID_StartNum);

	DDX_Text(pDX, IDE_MIN_ONLINETIME, m_nMinOnlineTime);
	DDV_MinMaxUInt(pDX, m_nMinOnlineTime, 0, 60);
	DDX_Text(pDX, IDE_MAX_ONLINETIME, m_nMaxOnlineTime);
	DDV_MinMaxUInt(pDX, m_nMaxOnlineTime, m_nMinOnlineTime, 120);
	DDX_Text(pDX, IDE_MIN_OFFLINETIME, m_nMinOfflineTime);
	DDV_MinMaxUInt(pDX, m_nMinOfflineTime, 0, 600);
	DDX_Text(pDX, IDE_MAX_OFFLINETIME, m_nMaxOfflineTime);
	DDV_MinMaxUInt(pDX, m_nMaxOfflineTime, m_nMinOfflineTime, 1200);
}

BEGIN_MESSAGE_MAP(CIOCP_ClientDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDB_START_TEST, &CIOCP_ClientDlg::OnBnClickedStartTest)
	ON_BN_CLICKED(IDB_STOP_TEST, &CIOCP_ClientDlg::OnBnClickedStopTest)
END_MESSAGE_MAP()

// CIOCP_ClientDlg ��Ϣ�������

BOOL CIOCP_ClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_nMinOfflineTime = 5;
	m_nMaxOfflineTime = 10;
	m_nMinOnlineTime = 5;
	m_nMaxOnlineTime = 10;
	UpdateData(FALSE);
	
	GetDlgItem(IDE_MAX_OFFLINETIME)->EnableWindow(FALSE);
	GetDlgItem(IDE_MAX_ONLINETIME)->EnableWindow(FALSE);
	GetDlgItem(IDE_MIN_OFFLINETIME)->EnableWindow(FALSE);
	GetDlgItem(IDE_MIN_ONLINETIME)->EnableWindow(FALSE);

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	GetDlgItem(IDB_STOP_TEST)->EnableWindow(FALSE);
	
	//��������IP��ַĬ����Ϊ58.67.148.65���˿�Ϊ88
	SetDlgItemText(IDC_IPADDRESS_SERVERIP,_T("192.168.11.112"));
	SetDlgItemInt(IDE_SERVER_PORT,83,FALSE);
	CheckDlgButton(IDRB_GETPUSHMSG,TRUE);
	
	 m_IOCPModel.Initalize();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CIOCP_ClientDlg::OnPaint()
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
HCURSOR CIOCP_ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CIOCP_ClientDlg::OnBnClickedStartTest()
{
	if (UpdateData(TRUE))
	{
		SetDlgItemInt(IDE_SESSION_NUM, 0, FALSE);
		m_nTestSpendTime = 0;
		SetDlgItemText(IDE_SPEND_TIME,_T("00:00"));
		SetTimer(ID_TIMER_PER_SEC, 1000,NULL);
		int iRadioButton = GetCheckedRadioButton(IDRB_GETCHALLENGE  , IDRB_GETPUSHMSG);
		switch (iRadioButton)
		{
		case IDRB_CONNET:
			m_testType = SocketConnect;
			break;
		case IDRB_GETCHALLENGE:
			m_testType = GetChallenge;
			break;
		case IDRB_REGISTER:
			m_testType = RegisterToken;
			break;
		case  IDRB_GETPUSHMSG:
			m_testType = GetPushMsg;
			break;
		}
		
		
		GetDlgItem(IDB_START_TEST)->EnableWindow(FALSE);
		GetDlgItem(IDB_STOP_TEST)->EnableWindow(TRUE);
		TSTRING strPerTokenID = m_strPreTokenID.GetBuffer();
		ClientDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));				// Sqlite ��ʼ����
		SetTimer(ID_TIMER_FRESH_INTERVAL, 100, NULL);

		// �����Զ������ߵ�ʱ�䷧ֵ��ÿ��Client���󶼻�������Random������ʱ�䣬�������ߺ�����
		TIME_THRESHOLD tm_threshold;
		tm_threshold.nMinOfflineIntervalTime = m_nMinOfflineTime;
		tm_threshold.nMaxOfflineIntervalTime = m_nMaxOfflineTime;
		tm_threshold.nMinOnlineIntervalTime = m_nMinOnlineTime;
		tm_threshold.nMaxOnlineIntervalTime = m_nMaxOnlineTime;

		srand(GetTickCount());
		m_IOCPModel.StartTest(m_dwServerIP, m_nServerPort, T2S(strPerTokenID).c_str(),
			m_TokenID_StartNum, m_nClientNum, m_testType, tm_threshold);
		m_strPreTokenID.ReleaseBuffer();

	}

}

void CIOCP_ClientDlg::OnBnClickedStopTest()
{
	UpdateData(TRUE);
	GetDlgItem(IDB_STOP_TEST)->EnableWindow(FALSE);
	
	m_IOCPModel.EndTest();
	ClientDB::GetInstance()->ExecuteSQL(TEXT("COMMIT;"));			// �ύ����
	KillTimer(ID_TIMER_FRESH_INTERVAL);
	OnTimer(ID_TIMER_FRESH_INTERVAL);
	KillTimer(ID_TIMER_PER_SEC);
	OnTimer(ID_TIMER_PER_SEC);
	MessageBox(_T("��������ɣ�"),_T("��ʾ"), MB_OK);
	GetDlgItem(IDB_START_TEST)->EnableWindow(TRUE);
}

void CIOCP_ClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (ID_TIMER_FRESH_INTERVAL == nIDEvent)
	{
		UINT nConCurrenceNum = m_IOCPModel.GetCurrentConnectNum();
		SetDlgItemInt(IDE_SESSION_NUM, nConCurrenceNum, FALSE);
		UINT nPushMsgNum = m_IOCPModel.GetRecvPushMsgNum();
		SetDlgItemInt(IDE_PUSHMSG_NUM, nPushMsgNum, FALSE);
	} 
	else if(ID_TIMER_PER_SEC == nIDEvent)
	{
		++m_nTestSpendTime;
		TCHAR szBuf[32] = {0};
		_stprintf(szBuf, _T("%02d:%02d"), m_nTestSpendTime/60, m_nTestSpendTime%60);
		SetDlgItemText(IDE_SPEND_TIME, szBuf);
	}

}