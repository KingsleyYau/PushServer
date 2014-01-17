// ServerPointDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ServerPoint.h"
#include "ServerPointDlg.h"
#include "LogFile.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IDTM_CYCLE_SEND_INTERVAL 1122

#define TIME_DIFF(start, end)	(start > end ? (((DWORD)-1) - start + end) : (end - start))


// CServerPointDlg �Ի���
const CString strLine(_T("\r\n---------------\r\n"));
const CString strSeparator(_T("\r\n========================\r\n"));


CServerPointDlg::CServerPointDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerPointDlg::IDD, pParent)
	, dwPushSerIP(0)
	, nPort(0)
	, m_strApplID(_T(""))
	, m_strAppKey(_T(""))
	, m_strTokenID(_T(""))
	, m_hThreadConn(NULL)
	, m_hThreadSend(NULL)
	, m_hThreadCycleSend(NULL)
	, m_nConnThreadID(0)
	, m_nCycleSendThreadID(0)
	, m_bConnState(FALSE)
	, m_strSource(_T(""))
	, m_strMD5(_T(""))
	, m_strContent(_T(""))
	, m_strMsg_Record(_T(""))
	, m_nSend_Interval(0)
	, m_bStopCycleSend(TRUE)
	//, m_CycleSendInterval_event(TRUE, FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerPointDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_SERVER_IP, dwPushSerIP);
	DDX_Text(pDX, IDE_PORT, nPort);
	DDX_Text(pDX, IDE_APPID, m_strApplID);
	DDX_Text(pDX, IDE_APPKEY, m_strAppKey);
	DDX_Text(pDX, IDE_TOKENID, m_strTokenID);
	DDX_Control(pDX, IDB_LOGIN_SERVER, m_btn_Connect);
	DDX_Control(pDX, IDB_SEND, m_btn_SendContent);
	DDX_Control(pDX, IDB_CLEAR_TEXT, m_btn_ClearText);
	DDX_Text(pDX, IDE_STRING, m_strSource);
	DDX_Text(pDX, IDE_STRING_MD5, m_strMD5);
	DDX_Control(pDX, IDE_SEND_CONTENT, m_edt_Content);
	DDX_Text(pDX, IDE_SEND_CONTENT, m_strContent);
	DDX_Text(pDX, IDE_MSG_RECORD, m_strMsg_Record);
	DDX_Control(pDX, IDE_MSG_RECORD, m_Edt_MsgRecord);
	DDX_Text(pDX, IDE_SEND_INTERVAL, m_nSend_Interval);
	//DDV_MinMaxInt(pDX, m_nSend_Interval, 0, 200);
	DDX_Control(pDX, IDB_CYCLE_SEND, m_btn_CycleSend);
	DDX_Control(pDX, IDB_STOP_CYCLE_SEND, m_btn_Stop_CycleSend);
}

BEGIN_MESSAGE_MAP(CServerPointDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	//ON_WM_TIMER()
	ON_BN_CLICKED(IDB_LOGIN_SERVER, &CServerPointDlg::OnBnClickedLoginServer)
	ON_MESSAGE(SUCCESS_CONNECT, &CServerPointDlg::OnSuccessConnect)
	ON_MESSAGE(ERROR_CONNECT, &CServerPointDlg::OnErrorConnect)
	ON_MESSAGE(SHOW_MSG2DLG, &CServerPointDlg::OnSHowMSG2Dlg)
	ON_BN_CLICKED(IDB_GENERATE_MD5, &CServerPointDlg::OnBnClickedGenerateMd5)
	ON_BN_CLICKED(IDB_SEND, &CServerPointDlg::OnBnClickedSend)
	ON_BN_CLICKED(IDB_CLEAR_TEXT, &CServerPointDlg::OnBnClickedClearText)
	ON_BN_CLICKED(IDB_CYCLE_SEND, &CServerPointDlg::OnBnClickedCycleSend)
	ON_BN_CLICKED(IDB_STOP_CYCLE_SEND, &CServerPointDlg::OnBnClickedStopCycleSend)
END_MESSAGE_MAP()


// CServerPointDlg ��Ϣ�������

BOOL CServerPointDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	CenterWindow();
	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	m_btn_SendContent.EnableWindow(FALSE);
	m_btn_ClearText.EnableWindow(FALSE);
	m_btn_CycleSend.EnableWindow(FALSE);
	m_btn_Stop_CycleSend.EnableWindow(FALSE);

	SetDlgItemText(IDC_SERVER_IP, _T("42.121.12.89"));
	SetDlgItemInt(IDE_PORT, 88);
	SetDlgItemText(IDE_APPID, _T("7f1171a78ce0780a2142a6eb7bc4f3c8"));
	SetDlgItemText(IDE_APPKEY, _T("a83a3f4a9186f1ba4298982691f0feff"));
	SetDlgItemInt(IDE_SEND_INTERVAL, -1, TRUE);
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CServerPointDlg::OnPaint()
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
HCURSOR CServerPointDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CServerPointDlg::OnBnClickedLoginServer()
{
	if (m_bConnState)
	{
		// ���������С���׼���Ͽ�
		
		// ��ֹ����
		m_btn_SendContent.EnableWindow(FALSE);
		m_btn_ClearText.EnableWindow(FALSE);
		m_btn_CycleSend.EnableWindow(FALSE);
		m_btn_Stop_CycleSend.EnableWindow(FALSE);

		// �Ͽ�����
		m_SrvPoint.DisConnect2PushSRV();		
		m_bConnState = FALSE;
		m_btn_Connect.SetWindowText(_T("����PushServer"));
		
	} 
	else
	{
		// �Ͽ�����׼������
		if (UpdateData(TRUE))
		{
			if (!m_strApplID.IsEmpty() &&
				!m_strAppKey.IsEmpty() 	)
			{	
				m_btn_Connect.EnableWindow(FALSE);
				m_btn_Connect.SetWindowText(_T("���������С���"));
				m_hThreadConn = ::CreateThread(NULL, 0, _ConnectThreadProc, this, 0, &m_nConnThreadID);
			}
		} 
		else
		{
			return;
		}
	}

}

LRESULT CServerPointDlg::OnSuccessConnect(WPARAM wParam, LPARAM lParam)
{
	WaitForSingleObject(m_hThreadConn, INFINITE);
	CloseHandle(m_hThreadConn);
	m_hThreadConn = NULL;
	m_bConnState =TRUE;			// �Ѿ��ɹ�������
	m_btn_Connect.SetWindowText(_T("�Ͽ�����"));
	m_btn_Connect.EnableWindow(TRUE);
	m_btn_SendContent.EnableWindow(TRUE);
	m_btn_ClearText.EnableWindow(TRUE);
	m_btn_CycleSend.EnableWindow(TRUE);
	m_btn_Stop_CycleSend.EnableWindow(TRUE);
	return 1;
}

LRESULT CServerPointDlg::OnErrorConnect(WPARAM wParam, LPARAM lParam)
{
	WaitForSingleObject(m_hThreadConn, INFINITE);
	CloseHandle(m_hThreadConn);
	m_hThreadConn = NULL;
	m_btn_Connect.SetWindowText(_T("����PushServer"));
	m_btn_Connect.EnableWindow(TRUE);
	return 1;
}
LRESULT CServerPointDlg:: OnSHowMSG2Dlg(WPARAM wParam, LPARAM lParam)
{
	UpdateData(FALSE);
	m_Edt_MsgRecord.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
	return 1;
}
// ���ӵ�PushServer
DWORD CServerPointDlg::_ConnectThreadProc(LPVOID lParam)
{
	CServerPointDlg *pDlg = reinterpret_cast<CServerPointDlg*>(lParam);

	// ��ServerEndPoint ������� AppID, AppKey, TokenID..
	pDlg->m_SrvPoint.SetAppInfo(T2S(pDlg->m_strApplID.GetBuffer()),
										T2S(pDlg->m_strAppKey.GetBuffer()));
	pDlg->m_strApplID.ReleaseBuffer();
	pDlg->m_strAppKey.ReleaseBuffer();
	
	// ���ӵ�PushServer
	BOOL bResult = pDlg->m_SrvPoint.Connect2PushSRV(pDlg->dwPushSerIP, pDlg->nPort);
	bResult = bResult && pDlg->m_SrvPoint.GetChallenge();
	bResult = bResult && pDlg->m_SrvPoint.LoginServer();
	pDlg->m_SrvPoint.AddListener(pDlg);

	if (bResult)
	{
		pDlg->PostMessage(SUCCESS_CONNECT, 0, 0);
	} 
	else
	{
		pDlg->PostMessage(ERROR_CONNECT, 0, 0);
	}
	return bResult;
}
// ѭ������PushMsg ��Ϣ�����߳� 
DWORD CServerPointDlg::_CycleSendPushMsgThreadProc(LPVOID lParam)
{
	CServerPointDlg *pDlg = reinterpret_cast<CServerPointDlg*>(lParam);
	int nCount = 0;

	pDlg->m_SrvPoint.SetTokenID(T2S((pDlg->m_strTokenID.GetBuffer())));
	pDlg->m_strTokenID.ReleaseBuffer();

	string strSendContent = T2S(pDlg->m_strContent.GetBuffer());
	pDlg->m_strContent.ReleaseBuffer();
	char szbuf[1024]  = {0};
	while (!pDlg->m_bStopCycleSend)
	{
		ZeroMemory(szbuf,sizeof(szbuf));
		sprintf_s(szbuf, sizeof(szbuf), strSendContent.c_str(), nCount);
		pDlg->m_SrvPoint.SendPushMessage(szbuf);
		++nCount;
		//WaitForSingleObject(pDlg->m_CycleSendInterval_event,INFINITE);
		DWORD dwNow = ::GetTickCount();

		while (!pDlg->m_bStopCycleSend && TIME_DIFF(dwNow, ::GetTickCount()) >= (DWORD)(pDlg->m_nSend_Interval))
		{
			Sleep(20);
		}
	}
	return 0;
}
void CServerPointDlg::OnBnClickedGenerateMd5()
{
	UpdateData(TRUE);
	if (m_strSource.IsEmpty())
	{
		// Դ�ַ���Ϊ��
		m_strMD5.Empty();
		UpdateData(FALSE);
		return ;
	} 
	else
	{
		CStringA strSource(m_strSource);
		string strMD5 = Md5HexString(strSource.GetBuffer());
		strSource.ReleaseBuffer();
		wstring wstrMD5 = S2T(strMD5);
		m_strMD5.Empty();
		m_strMD5.Format(L"%s", wstrMD5.c_str());
		UpdateData(FALSE);
	}
}

void CServerPointDlg::OnBnClickedSend()
{
	UpdateData(TRUE);
	if (m_strContent.IsEmpty() || m_strTokenID.IsEmpty() )
	{
		MessageBox(_T("TokenID�����ݲ���Ϊ��"), _T("ע�⣡"), MB_ICONWARNING | MB_OK);
		return;
	}
	m_SrvPoint.SetTokenID(T2S((m_strTokenID.GetBuffer())));
	m_strTokenID.ReleaseBuffer();
	m_SrvPoint.SendPushMessage(T2S(m_strContent.GetBuffer()));
	m_strContent.ReleaseBuffer();
}

void CServerPointDlg::OnSuccessSend(string strMsg)
{
	CString strSendMsg;
	strSendMsg.Format(L"%s", S2T(strMsg).c_str());
	m_strMsg_Record += strSendMsg;
	m_strMsg_Record += strLine;
	PostMessage(SHOW_MSG2DLG,0,0);
	//UpdateData(FALSE);
	//m_Edt_MsgRecord.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}
void CServerPointDlg::OnErrorSend(string strMsg)
{
	CString strSendMsg;
	strSendMsg.Format(L"%s", S2T(strMsg).c_str());
	m_strMsg_Record += strSendMsg;
	m_strMsg_Record += strLine;
	PostMessage(SHOW_MSG2DLG,0,0);
	//UpdateData(FALSE);
	//m_Edt_MsgRecord.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}
void CServerPointDlg::OnSuccessRecv(string strMsg)
{
	CString strRecvMsg;
	strRecvMsg.Format(L"%s", S2T(strMsg).c_str());
	m_strMsg_Record += strRecvMsg;
	m_strMsg_Record += strSeparator;
	//UpdateData(FALSE);
	//m_Edt_MsgRecord.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}
void CServerPointDlg::OnBnClickedClearText()
{
	m_strContent.Empty();
	UpdateData(FALSE);
}

void CServerPointDlg::OnBnClickedCycleSend()
{
	UpdateData(TRUE);
	if (m_strContent.IsEmpty() || m_strTokenID.IsEmpty() )
	{
		MessageBox(_T("TokenID�����ݲ���Ϊ��"), _T("ע�⣡"), MB_ICONWARNING | MB_OK);
		return;
	}
	if (m_nSend_Interval < 0 ||
		m_nSend_Interval >200)
	{
		MessageBox(_T("ѭ�����͵�ʱ�������ô������޸ģ�(��Χ��0-200)"),_T("ע�⣡"), MB_ICONWARNING | MB_OK);
		return;
	}
	// �ж�Ҫ���͵������У���û�к�%d
	// ����ʼ���ͣ�������ʾ
	if (m_strContent.Find(_T("%d")) == -1)
	{
		// û���ҵ�%d
		MessageBox(_T("Ҫ���͵������У�û���ҵ�%d�����޸ģ�"), _T("ע�⣡"), MB_ICONWARNING | MB_OK);
		return;
	}
	// ����������ҵ�
	m_btn_SendContent.EnableWindow(FALSE);
	m_btn_ClearText.EnableWindow(FALSE);
	m_btn_CycleSend.EnableWindow(FALSE);
	m_btn_Stop_CycleSend.EnableWindow(TRUE);

	m_SrvPoint.SetTokenID(T2S((m_strTokenID.GetBuffer())));
	m_strTokenID.ReleaseBuffer();
	m_SrvPoint.SendPushMessage(T2S(m_strContent.GetBuffer()));
	m_strContent.ReleaseBuffer();
	// TODO: �ڴ���ӿؼ�֪ͨ����������
//	SetTimer(IDTM_CYCLE_SEND_INTERVAL, m_nSend_Interval*1000,NULL);
	m_bStopCycleSend = FALSE;
	m_hThreadCycleSend = ::CreateThread(NULL, 0, _CycleSendPushMsgThreadProc, this, 0, &m_nCycleSendThreadID);

}
//void CServerPointDlg::OnTimer(UINT_PTR nIDEvent)
//{
//	m_CycleSendInterval_event.SetEvent();	
//}

void CServerPointDlg::OnBnClickedStopCycleSend()
{
//	KillTimer(IDTM_CYCLE_SEND_INTERVAL);
	m_bStopCycleSend = TRUE;
//	m_CycleSendInterval_event.SetEvent();
	// ������
	WaitForSingleObject(m_hThreadCycleSend, 1500);

	DWORD dwExitCode = 0;
	GetExitCodeThread(m_hThreadCycleSend, &dwExitCode);
	if (STILL_ACTIVE==dwExitCode)
	{
		TerminateThread(m_hThreadCycleSend, 0);
	}
	CloseHandle(m_hThreadCycleSend);
	m_hThreadCycleSend = NULL;
	m_btn_SendContent.EnableWindow(TRUE);
	m_btn_CycleSend.EnableWindow(TRUE);
	m_btn_ClearText.EnableWindow(TRUE);
}
