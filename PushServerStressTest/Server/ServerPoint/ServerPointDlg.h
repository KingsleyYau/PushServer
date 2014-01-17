// ServerPointDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "ServerEndPoint.h"
#include "afxmt.h"

#define SUCCESS_CONNECT WM_USER +0x101
#define ERROR_CONNECT SUCCESS_CONNECT +1
#define  SHOW_MSG2DLG ERROR_CONNECT+1

// CServerPointDlg �Ի���
class CServerPointDlg : public CDialog, public IServerEndPointNotify
{
// ����
public:
	CServerPointDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SERVERPOINT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��



// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
//	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnSuccessConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnErrorConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSHowMSG2Dlg(WPARAM wParam, LPARAM lParam);


	// IServerEndPointNotify�ӿ�
	void OnSuccessSend(string strMsg);
	void OnErrorSend(string strMsg);
	void OnSuccessRecv(string strMsg);
private:
	
	DWORD dwPushSerIP;		// Push������IP
	UINT nPort;						// PushServer�˿�
	CString m_strApplID;
	CString m_strAppKey;
	CString m_strTokenID;
	int m_nSend_Interval;			// ���ͼ������λ���룩

	CString m_strSource;			// Դ�ַ���
	CString m_strMD5;				// MD5�ַ���

	CButton m_btn_Connect;				// ����
	CButton m_btn_SendContent;		// ������
	CButton m_btn_ClearText;			// �������
	CButton m_btn_CycleSend;			// ѭ�����Ͱ�ť
	CButton m_btn_Stop_CycleSend;	// ֹͣѭ������

	// Ҫ���͵�����
	CEdit m_edt_Content;
	CEdit m_Edt_MsgRecord;
	CString m_strContent;

	CString m_strMsg_Record;			// ��ʾ�ѷ��ͼ��յ�������

	afx_msg void OnBnClickedLoginServer();

	// �̴߳�����
	static DWORD WINAPI _ConnectThreadProc(LPVOID lParam);
	static DWORD WINAPI _SendDataThreadProc(LPVOID lParam);
	static DWORD WINAPI _CycleSendPushMsgThreadProc(LPVOID lParam);

	HANDLE m_hThreadConn;
	HANDLE m_hThreadSend;
	HANDLE m_hThreadCycleSend;
	BOOL  m_bStopCycleSend;
	//CEvent	m_CycleSendInterval_event;

	CServerEndPoint m_SrvPoint;
	BOOL m_bConnState;							// ��ǰCServerEndPoint ������״̬
	DWORD m_nConnThreadID;
	DWORD m_nCycleSendThreadID;
	afx_msg void OnBnClickedGenerateMd5();			// ����MD5
	afx_msg void OnBnClickedSend();						// ����
	afx_msg void OnBnClickedClearText();					// ���
	afx_msg void OnBnClickedCycleSend();				// ѭ������
	afx_msg void OnBnClickedStopCycleSend();			// ֹͣѭ������

};
