// PushServerStressTest_ClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PushServerStressTest_Client.h"
#include "PushServerStressTest_ClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPushServerStressTest_ClientDlg 对话框

CPushServerStressTest_ClientDlg::CPushServerStressTest_ClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPushServerStressTest_ClientDlg::IDD, pParent)
	, m_dwServerIP(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPushServerStressTest_ClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_TEST, m_TabCtrl);
	DDX_IPAddress(pDX, IDC_SER_IPADDR, m_dwServerIP);
}

BEGIN_MESSAGE_MAP(CPushServerStressTest_ClientDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_TEST, &CPushServerStressTest_ClientDlg::OnTcnSelchangeTabTest)
	ON_MESSAGE(WM_GET_PUSHSRV_IP,&CPushServerStressTest_ClientDlg::OnGetPushServerIP)
	ON_MESSAGE(WM_GET_PUSHSRV_PORT,&CPushServerStressTest_ClientDlg::OnGetPushServerPort)
END_MESSAGE_MAP()


// CPushServerStressTest_ClientDlg 消息处理程序

BOOL CPushServerStressTest_ClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//将服务器IP地址默认设为58.67.148.65，端口为88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.0.176"));
	SetDlgItemInt(IDE_SER_PORT,88,FALSE);

	// Tab 初始化
	m_TabCtrl.InsertItem(0, _T("常规测试"));
	m_TabCtrl.InsertItem(1, _T("压力测试"));

	// 两个Tab页
	m_dlgRouTineTest.Create(IDD_ROUTINE_TEST, this);
	m_dlgStressTest.Create(IDD_STRESS_TEST, this);

	CRect rcTabCtrlClient;
	m_TabCtrl.GetClientRect(&rcTabCtrlClient);
	rcTabCtrlClient.top += 20;
	rcTabCtrlClient.bottom -= 2;
	rcTabCtrlClient.left += 2;
	rcTabCtrlClient.right -= 2;

	//m_TabCtrl.MapWindowPoints(this,rcTabCtrlClient)
	
	m_TabCtrl.MapWindowPoints(this,&rcTabCtrlClient);
	m_dlgRouTineTest.MoveWindow(rcTabCtrlClient);
	m_dlgStressTest.MoveWindow(rcTabCtrlClient);

	// 默认显示常规测试
	m_TabCtrl.SetCurSel( 0 );
	m_dlgRouTineTest.ShowWindow(TRUE);
	//m_dlgStressTest.ShowWindow(TRUE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPushServerStressTest_ClientDlg::OnPaint()
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
HCURSOR CPushServerStressTest_ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPushServerStressTest_ClientDlg::OnTcnSelchangeTabTest(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	switch(m_TabCtrl.GetCurSel())
	{
	case 0:
		{
			m_dlgRouTineTest.ShowWindow(TRUE);
			m_dlgStressTest.ShowWindow(FALSE);
		}
		break;
	case 1:
		{
			m_dlgRouTineTest.ShowWindow(FALSE);
			m_dlgStressTest.ShowWindow(TRUE);
		}
		break;
	}
	*pResult = 0;
}

// 自定义消息处理函数

LRESULT CPushServerStressTest_ClientDlg::OnGetPushServerIP(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);
	// 获得当前设置的IP
	LPDWORD dwServerIP = reinterpret_cast<LPDWORD>(lParam);
	*dwServerIP = m_dwServerIP;

	return TRUE;
}
LRESULT CPushServerStressTest_ClientDlg::OnGetPushServerPort(WPARAM wParam, LPARAM lParam)
{
	// 获得当前设置的端口
	PUINT nServerPort = reinterpret_cast<PUINT>(lParam);
	*nServerPort = GetDlgItemInt(IDE_SER_PORT);
	return TRUE;
}