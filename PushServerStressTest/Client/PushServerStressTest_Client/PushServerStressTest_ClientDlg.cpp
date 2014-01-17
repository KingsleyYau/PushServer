// PushServerStressTest_ClientDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PushServerStressTest_Client.h"
#include "PushServerStressTest_ClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPushServerStressTest_ClientDlg �Ի���

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


// CPushServerStressTest_ClientDlg ��Ϣ�������

BOOL CPushServerStressTest_ClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//��������IP��ַĬ����Ϊ58.67.148.65���˿�Ϊ88
	SetDlgItemText(IDC_SER_IPADDR,_T("192.168.0.176"));
	SetDlgItemInt(IDE_SER_PORT,88,FALSE);

	// Tab ��ʼ��
	m_TabCtrl.InsertItem(0, _T("�������"));
	m_TabCtrl.InsertItem(1, _T("ѹ������"));

	// ����Tabҳ
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

	// Ĭ����ʾ�������
	m_TabCtrl.SetCurSel( 0 );
	m_dlgRouTineTest.ShowWindow(TRUE);
	//m_dlgStressTest.ShowWindow(TRUE);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPushServerStressTest_ClientDlg::OnPaint()
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
HCURSOR CPushServerStressTest_ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPushServerStressTest_ClientDlg::OnTcnSelchangeTabTest(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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

// �Զ�����Ϣ������

LRESULT CPushServerStressTest_ClientDlg::OnGetPushServerIP(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);
	// ��õ�ǰ���õ�IP
	LPDWORD dwServerIP = reinterpret_cast<LPDWORD>(lParam);
	*dwServerIP = m_dwServerIP;

	return TRUE;
}
LRESULT CPushServerStressTest_ClientDlg::OnGetPushServerPort(WPARAM wParam, LPARAM lParam)
{
	// ��õ�ǰ���õĶ˿�
	PUINT nServerPort = reinterpret_cast<PUINT>(lParam);
	*nServerPort = GetDlgItemInt(IDE_SER_PORT);
	return TRUE;
}