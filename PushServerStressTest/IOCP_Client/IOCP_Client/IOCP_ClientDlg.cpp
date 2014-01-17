// IOCP_ClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IOCP_Client.h"
#include "IOCP_ClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIOCP_ClientDlg 对话框

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


// CIOCP_ClientDlg 消息处理程序

BOOL CIOCP_ClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_nMinOfflineTime = 5;
	m_nMaxOfflineTime = 10;
	m_nMinOnlineTime = 5;
	m_nMaxOnlineTime = 10;
	UpdateData(FALSE);
	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	GetDlgItem(IDB_STOP_TEST)->EnableWindow(FALSE);
	
	//将服务器IP地址默认设为58.67.148.65，端口为88
	SetDlgItemText(IDC_IPADDRESS_SERVERIP,_T("192.168.12.130"));
	SetDlgItemInt(IDE_SERVER_PORT,88,FALSE);


	 m_IOCPModel.Initalize();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CIOCP_ClientDlg::OnPaint()
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
HCURSOR CIOCP_ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CIOCP_ClientDlg::OnBnClickedStartTest()
{
	if (UpdateData(TRUE))
	{
		GetDlgItem(IDB_START_TEST)->EnableWindow(FALSE);
		GetDlgItem(IDB_STOP_TEST)->EnableWindow(TRUE);
		TSTRING strPerTokenID = m_strPreTokenID.GetBuffer();
		ClientDB::GetInstance()->ExecuteSQL(TEXT("BEGIN;"));				// Sqlite 开始事务
		SetTimer(1012, 100, NULL);

		// 设置自动上下线的时间阀值，每个Client对象都会从中随机Random出两个时间，用于上线和下线
		TIME_THRESHOLD tm_threshold;
		tm_threshold.nMinOfflineIntervalTime = m_nMinOfflineTime;
		tm_threshold.nMaxOfflineIntervalTime = m_nMaxOfflineTime;
		tm_threshold.nMinOnlineIntervalTime = m_nMinOnlineTime;
		tm_threshold.nMaxOnlineIntervalTime = m_nMaxOnlineTime;

		srand(GetTickCount());
		m_IOCPModel.StartTest(m_dwServerIP, m_nServerPort, T2S(strPerTokenID).c_str(),
			m_TokenID_StartNum, m_nClientNum, tm_threshold);
		m_strPreTokenID.ReleaseBuffer();

	}

}

void CIOCP_ClientDlg::OnBnClickedStopTest()
{
	UpdateData(TRUE);
	GetDlgItem(IDB_STOP_TEST)->EnableWindow(FALSE);
	
	m_IOCPModel.EndTest();
	ClientDB::GetInstance()->ExecuteSQL(TEXT("COMMIT;"));			// 提交事务
	KillTimer(1012);
	OnTimer(1012);
	MessageBox(_T("测试已完成！"),_T("提示"), MB_OK);
	GetDlgItem(IDB_START_TEST)->EnableWindow(TRUE);
}

void CIOCP_ClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	UINT nConCurrenceNum = m_IOCPModel.GetCurrentConnectNum();
	SetDlgItemInt(IDE_SESSION_NUM, nConCurrenceNum, FALSE);
	UINT nPushMsgNum = m_IOCPModel.GetRecvPushMsgNum();
	SetDlgItemInt(IDE_PUSHMSG_NUM, nPushMsgNum, FALSE);
}