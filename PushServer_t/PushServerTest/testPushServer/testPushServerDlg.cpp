// testPushServerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "testPushServer.h"
#include "testPushServerDlg.h"
//#include "MD5.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CtestPushServerDlg 对话框

//#define SERVERIP	_T("192.168.11.113")
//#define SERVERPORT	83

#define SERVERIP	_T("58.67.148.65")
#define SERVERPORT	88
//
//#define SERVERIP	_T("42.121.12.89")
//#define SERVERPORT	88

#define CHANLLENGE_DEF	"\"chanllenge\":\""
#define STRINGEND_DEF	"\""


#define LOOPSENDTIMER_ID	101
#define LOOPSENDINTVL		5000	// n毫秒/次

CtestPushServerDlg::CtestPushServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestPushServerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	m_bLoopSend = FALSE;
	m_iSendCount = 0;

	m_hConnectThread = NULL;
	m_bStartConnect = FALSE;
	m_connSocket = 0;

	m_bStartSendMsg = FALSE;
}

CtestPushServerDlg::~CtestPushServerDlg()
{
	WSACleanup();
}

void CtestPushServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_RECV2, m_editRecv);
	DDX_Control(pDX, IDC_EDIT_SEND, m_editSend);
}

BEGIN_MESSAGE_MAP(CtestPushServerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CtestPushServerDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_CLEAN, &CtestPushServerDlg::OnBnClickedButtonClean)
	ON_BN_CLICKED(IDC_BTN_LOOPSEND, &CtestPushServerDlg::OnBnClickedBtnLoopsend)
	ON_BN_CLICKED(IDC_BTN_ALWAYSCONNECT, &CtestPushServerDlg::OnBnClickedBtnAlwaysconnect)
	ON_BN_CLICKED(IDC_BTN_SENDONEMSG, &CtestPushServerDlg::OnBnClickedBtnSendonemsg)
END_MESSAGE_MAP()


// CtestPushServerDlg 消息处理程序

BOOL CtestPushServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_editRecv.SetReadOnly();
	m_editRecv.SetLimitText(10000);
	m_editSend.SetLimitText(10000);

	// socket
	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	
	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr("");
	client.sin_port = htons(0);
	if (bind( m_socket, (SOCKADDR*) &client, sizeof(client) ) == SOCKET_ERROR)
	{
		::MessageBox(m_hWnd, "bind error!", "", MB_OK);
	}

	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(SERVERIP);
	server.sin_port = htons(SERVERPORT);
	if (::connect(m_socket, (const sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		::MessageBox(m_hWnd, "connect error!", "", MB_OK);
	}

	m_hRecvThread = ::CreateThread(NULL, 0, RecvThread, this, 0, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CtestPushServerDlg::OnClose()
{
	::shutdown(m_socket, SD_BOTH);
	::closesocket(m_socket);
	
	::WaitForSingleObject(m_hRecvThread, INFINITE);
	::CloseHandle(m_hRecvThread);

	CDialog::OnClose();
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CtestPushServerDlg::OnPaint()
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
HCURSOR CtestPushServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD CtestPushServerDlg::RecvThread(LPVOID lpObj)
{
	CtestPushServerDlg *pDlg = (CtestPushServerDlg*)lpObj;
	const int nBufferSize = 10240;
	char cBuffer[nBufferSize] = {0};
	int nRecvSize = 0;
	while (true)
	{
		nRecvSize = ::recv(pDlg->m_socket, cBuffer, nBufferSize, 0);
		if (nRecvSize > 0) {
			cBuffer[nRecvSize] = 0;
			pDlg->RecvThreadProc(cBuffer);
		}
		else {
			::MessageBox(NULL, _T("发送失败，连接已经断开"), _T("错误"), MB_OK);
			break;
		}
	}
	return 0;
}

void CtestPushServerDlg::RecvThreadProc(const char* data)
{
	DWORD dwSel = m_editRecv.GetSel();
	m_editRecv.SetSel(-1, -1);
	m_editRecv.ReplaceSel(data);
	m_editRecv.SetSel(dwSel);
	m_editRecv.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}
void CtestPushServerDlg::OnBnClickedButtonSend()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSend;
	m_editSend.GetWindowText(strSend);
	::send(m_socket, (LPCSTR)strSend, strSend.GetLength(), 0);
	m_editSend.SetWindowText("");
}

void CtestPushServerDlg::OnBnClickedButtonClean()
{
	// TODO: 在此添加控件通知处理程序代码
	m_editRecv.SetSel(0, -1);
	m_editRecv.ReplaceSel("");
}

const TCHAR* g_strHttpHead = _T("POST / HTTP/1.1\r\nCHARSET: UTF-8\r\nCONNECTION: KEEP-ALIVE\r\nCONTENT-TYPE: TEXT/HTML\r\nUSER-AGENT: DALVIK/1.4.0 (LINUX; U; ANDROID 2.3.6; MB526 BUILD/4.5.1-134_DFP-231)\r\nHOST: 192.168.11.113:81\r\nCONTENT-LENGTH: %d\r\nACCEPT-ENCODING: GZIP\r\n\r\n");
const TCHAR* g_strJsonContant= _T("jsonparam={\"cmd\":\"pushmsg\",\"body\":{\"tokenidlist\":[{\"tokenid\":\"%s\"}],\"body\":{\"aps\":{\"alert\":\"test%d\",\"badge\":%d,\"sound\":\"default\"},\"etype\":1}}}");

void CtestPushServerDlg::OnBnClickedBtnLoopsend()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bLoopSend) {
		// 未进行重复发送
		m_bLoopSend = TRUE;
		m_iSendCount = 0;
		m_editSend.SetWindowText(_T(""));
		m_editSend.EnableWindow(FALSE);

		CString strIntval;
		GetDlgItemText(IDC_EDIT_INVAL, strIntval);
		SetTimer(LOOPSENDTIMER_ID, _ttoi(strIntval), NULL);
	}
	else {
		// 已经重复发送
		m_bLoopSend = FALSE;
		m_editSend.EnableWindow(TRUE);
		m_editSend.SetWindowText(_T(""));
		KillTimer(LOOPSENDTIMER_ID);
	}
}

void CtestPushServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (LOOPSENDTIMER_ID == nIDEvent) {
		// 重复发送
		OnBnClickedButtonClean();

		CString strTokenId;
		GetDlgItemText(IDC_EDIT_TOKENID, strTokenId);
		TCHAR szBody[1024] = {0};
		_stprintf_s(szBody, _countof(szBody), g_strJsonContant, strTokenId, m_iSendCount, m_iSendCount);

		TCHAR szHead[1024] = {0};
		_stprintf_s(szHead, _countof(szHead), g_strHttpHead, _tcslen(szBody));

		CString strData = szHead;
		strData += szBody;
		
		m_editSend.EnableWindow(TRUE);
		m_editSend.SetWindowText(strData);
		m_editSend.EnableWindow(FALSE);
		OnBnClickedButtonSend();

		strData += _T("\r\n---------------\r\n");
		RecvThreadProc(strData);

		m_iSendCount++;
	}
}

void CtestPushServerDlg::OnBnClickedBtnAlwaysconnect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bStartConnect)
	{
		m_bStartConnect = TRUE;
		m_hConnectThread = ::CreateThread(NULL, 0, ConnectThread, this, 0, NULL);
		m_bStartConnect = NULL != m_hConnectThread;
	}
	else 
	{
		m_bStartConnect = FALSE;
		::closesocket(m_connSocket);
		::WaitForSingleObject(m_hConnectThread, INFINITE);
		::CloseHandle(m_hConnectThread);
		m_hConnectThread = NULL;
	}
}

DWORD WINAPI CtestPushServerDlg::ConnectThread(LPVOID lpObj)
{
	CtestPushServerDlg *pThis = (CtestPushServerDlg*)lpObj;
	pThis->ConnectThreadProc();
	return 0;
}
	
void CtestPushServerDlg::ConnectThreadProc()
{
	while (m_bStartConnect)
	{
		CString strSend;

		// socket
		m_connSocket = ::socket(AF_INET, SOCK_STREAM, 0);
		
		//sockaddr_in client;
		//client.sin_family = AF_INET;
		//client.sin_addr.s_addr = inet_addr("");
		//client.sin_port = htons(0);
		//if (bind( m_connSocket, (SOCKADDR*) &client, sizeof(client) ) == SOCKET_ERROR)
		//{
		//	goto CONNECTTHEEND;
		//}

		sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr(SERVERIP);
		server.sin_port = htons(SERVERPORT);
		if (::connect(m_connSocket, (const sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
		{
			goto CONNECTTHEEND;
		}

		static bool bSendTrueProtocol = true;
		strSend = _T("POST / HTTP/1.1\r\n");
		strSend += _T("CONNECTION: KEEP-ALIVE\r\n");
		strSend += _T("CONTENT-LENGTH: 29\r\n\r\n");
		strSend += _T("{\"cmd\":\"getclientchanllenge\"}");
		if (bSendTrueProtocol)
		{
			::send(m_connSocket, (LPCSTR)strSend, strSend.GetLength(), 0);
		}
		else
		{
			for (int i = 0; i < 100; i++) {
				strSend += _T("{\"cmd\":\"getclientchanllenge\"}");
			}
			int sendlength = 0;
			int totlelength = 0;
			while (totlelength != strSend.GetLength() && sendlength >= 0) {

				sendlength = ::send(m_connSocket, (LPCSTR)strSend, strSend.GetLength(), 0);
				totlelength += sendlength;
				::Sleep(10);
			}
		}
		bSendTrueProtocol = !bSendTrueProtocol;

		const int nBufferSize = 10240;
		static char cBuffer[nBufferSize] = {0};
		int nRecvSize = 0;
		nRecvSize = ::recv(m_connSocket, cBuffer, nBufferSize, 0);
		if (nRecvSize > 0) 
		{
			cBuffer[nBufferSize] = 0;
			::SendMessage(m_hWnd, WM_COMMAND, MAKELONG(IDC_BUTTON_CLEAN, BN_CLICKED), (LPARAM)::GetDlgItem(m_hWnd, IDC_BUTTON_CLEAN));
			RecvThreadProc(cBuffer);
		}

CONNECTTHEEND:
		::closesocket(m_connSocket);
		m_connSocket = 0;
		::Sleep(500);
	}
}

void CtestPushServerDlg::OnBnClickedBtnSendonemsg()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bStartSendMsg)
	{
		m_bStartSendMsg = TRUE;
		m_hSendMsgThread = ::CreateThread(NULL, 0, SendMsgThread, this, 0, NULL);
		m_bStartSendMsg = NULL != m_hSendMsgThread;
	}
	else 
	{
		m_bStartSendMsg = FALSE;
		::closesocket(m_sendmsgSocket);
		::WaitForSingleObject(m_hSendMsgThread, INFINITE);
		::CloseHandle(m_hSendMsgThread);
		m_hSendMsgThread = NULL;
	}
}

DWORD WINAPI CtestPushServerDlg::SendMsgThread(LPVOID lpObj)
{
	CtestPushServerDlg *pThis = (CtestPushServerDlg*)lpObj;
	pThis->SendMsgThreadProc();
	return 0;
}
	
void CtestPushServerDlg::SendMsgThreadProc()
{
//	while (m_bStartSendMsg)
//	{
//		CString strSend;
//
//		// socket
//		m_connSocket = ::socket(AF_INET, SOCK_STREAM, 0);
//		
//		sockaddr_in client;
//		client.sin_family = AF_INET;
//		client.sin_addr.s_addr = inet_addr("");
//		client.sin_port = htons(0);
//		if (bind( m_connSocket, (SOCKADDR*) &client, sizeof(client) ) == SOCKET_ERROR)
//		{
//			goto SENDMSGTHEEND;
//		}
//
//		sockaddr_in server;
//		server.sin_family = AF_INET;
//		server.sin_addr.s_addr = inet_addr(SERVERIP);
//		server.sin_port = htons(SERVERPORT);
//		if (::connect(m_connSocket, (const sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
//		{
//			goto SENDMSGTHEEND;
//		}
//
//		// ------------ Get chanllenge ----------------
//		strSend = _T("POST / HTTP/1.1\r\n");
//		strSend += _T("CONNECTION: KEEP-ALIVE\r\n");
//		strSend += _T("CONTENT-LENGTH: 29\r\n\r\n");
//		strSend += _T("{\"cmd\":\"getserverchanllenge\"}");
//		if (bSendTrueProtocol)
//		{
//			::send(m_connSocket, (LPCSTR)strSend, strSend.GetLength(), 0);
//		}
//		
//		const int nBufferSize = 10240;
//		static char cBuffer[nBufferSize] = {0};
//		int nRecvSize = 0;
//		nRecvSize = ::recv(m_connSocket, cBuffer, nBufferSize, 0);
//		if (nRecvSize <= 0) 
//		{
//			goto SENDMSGTHEEND;
//		}
//
//		const char* szBeginChanllenge = strstr(cBuffer, CHANLLENGE_DEF);
//		if (NULL == szBeginChanllenge) {
//			goto SENDMSGTHEEND;
//		}
//		const char* szEndChanllenge = strstr(szBeginChanllenge, STRINGEND_DEF);
//		if (NULL == szEndChanllenge) {
//			goto SENDMSGTHEEND;
//		}
//		szBeginChanllenge += strlen(CHANLLENGE_DEF);
//		CString strChanllenge(szBeginChanllenge, szEndChanllenge - szBeginChanllenge);
//
//		// ------------ Register ----------------
//		CString strMd5Code = strChanllenge + _T("a83a3f4a9186f1ba4298982691f0feff");
//		string strCheckCode = Md5HexString(strMd5Code);
//
//		strSend = _T("POST / HTTP/1.1\r\n");
//		strSend += _T("CONNECTION: KEEP-ALIVE\r\n");
//		strSend += _T("CONTENT-LENGTH: 127\r\n\r\n");
//		strSend += _T("{\"cmd\":\"register\",\"body\":{\"appid\":\"7f1171a78ce0780a2142a6eb7bc4f3c8\",\"checkcode\":\"");
//		strSend += strCheckCode.c_str();
//		strSend += _T("\"}}");
//		if (bSendTrueProtocol)
//		{
//			::send(m_connSocket, (LPCSTR)strSend, strSend.GetLength(), 0);
//		}
//		
//		memset(cBuffer, 0, nBufferSize);
//		nRecvSize = ::recv(m_connSocket, cBuffer, nBufferSize, 0);
//		if (nRecvSize <= 0) 
//		{
//			goto SENDMSGTHEEND;
//		}
//
//		if (NULL == _tcsstr(cBuffer, _T("\"ret\":0")))
//		{
//			goto SENDMSGTHEEND;
//		}
//
//SENDMSGTHEEND:
//		::closesocket(m_connSocket);
//		m_connSocket = 0;
//		::Sleep(500);
//	}
}