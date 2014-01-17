// testPushServerDlg.h : ͷ�ļ�
//

#pragma once
#include <afxwin.h>
#include <winsock2.h>


// CtestPushServerDlg �Ի���
class CtestPushServerDlg : public CDialog
{
// ����
public:
	CtestPushServerDlg(CWnd* pParent = NULL);	// ��׼���캯��
	virtual ~CtestPushServerDlg();

// �Ի�������
	enum { IDD = IDD_TESTPUSHSERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

protected:
	static DWORD WINAPI RecvThread(LPVOID lpObj);
	void RecvThreadProc(const char* data);

	static DWORD WINAPI ConnectThread(LPVOID lpObj);
	void ConnectThreadProc();

	static DWORD WINAPI SendMsgThread(LPVOID lpObj);
	void SendMsgThreadProc();

public:
	SOCKET	m_socket;

private:
	CEdit m_editSend;
	CEdit m_editRecv;
	HANDLE	m_hRecvThread;
	int		m_iSendCount;

	HANDLE	m_hConnectThread;
	BOOL	m_bStartConnect;
	SOCKET	m_connSocket;

	HANDLE	m_hSendMsgThread;
	BOOL	m_bStartSendMsg;
	SOCKET	m_sendmsgSocket;

private:
	CString	m_strLoopSendHead;
	CString	m_strLoopSendContant;
	BOOL	m_bLoopSend;
	
public:
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonClean();
	afx_msg void OnBnClickedBtnLoopsend();
	afx_msg void OnBnClickedBtnAlwaysconnect();
	afx_msg void OnBnClickedBtnSendonemsg();
};
