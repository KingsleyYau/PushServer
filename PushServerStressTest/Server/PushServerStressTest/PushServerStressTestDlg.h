// PushServerStressTestDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include <WinSock2.h>
#include "ServerEndPoint.h"
#include "afxcmn.h"
#include <afxmt.h>
// CPushServerStressTestDlg �Ի���
class CPushServerStressTestDlg : public CDialog
{
// ����
public:
	CPushServerStressTestDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CPushServerStressTestDlg();

// �Ի�������
	enum { IDD = IDD_PUSHSERVERSTRESSTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
//	afx_msg void OnStartTimer();
//	afx_msg void OnStopTimer();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()


// �̴߳�����
	
	static DWORD WINAPI Thread_I_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_II_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_III_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_IV_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_Stress_Test(LPVOID lParam);


//��Ա����
private:
	//��������
	DWORD m_dwServerIP;			// Push������IP
	UINT m_nServerPort;				// Push�������˿�
	CString m_strSendContent;		// ��Push���������͵�����

	//�ؼ���������

	CDialog *m_TabPage;
	CEdit m_SendContent;		


	// �ĸ������߳̾��

	HANDLE m_hThreadTest_I;
	HANDLE m_hThreadTest_II;
	HANDLE m_hThreadTest_III;
	HANDLE m_hThreadTest_IV;

	// Stress Test(ѭ����PushMessage�߳�)
	HANDLE m_hThreadStressTest;
	HANDLE m_hEventPauseTest;
	HANDLE m_hEventMinuteLock;
	// ��־�ĸ��߳��Ƿ�����
	BOOL m_bIsThread_I_Run;
	BOOL m_bIsThread_II_Run;
	BOOL m_bIsThread_III_Run;
	BOOL m_bIsThread_IV_Run;
	
	BOOL m_bIsThread_Test_Run;

	//CServerEndPoint SRVEndPoint;

	//  ���Ե�ʱ����

	UINT m_nTimeslot;

public:
	//afx_msg void OnBnClickedConn();

	//CTabCtrl m_TabCtrl;

	// �����߳�I
	CButton m_btnStartThread_I;
	// ֹͣ�߳�1
	CButton m_btnStopThread_I;
	// �����߳�II
	CButton m_btnStartThread_II;
	// ֹͣ�߳�II
	CButton m_btnStopThread_II;
	// �����߳�III
	CButton m_btnStartThread_III;
	// ֹͣ�߳�III
	CButton m_btnStopThread_III;
	// �����߳�IV
	CButton m_btnStartThread_IV;
	// ֹͣ�߳�IV
	CButton m_btnStopThread_IV;


	afx_msg void OnBnClickedStartThread();
	afx_msg void OnBnClickedStopThread();

	afx_msg void OnBnClickedStartThreadIi();
	afx_msg void OnBnClickedStopThreadIi();

	afx_msg void OnBnClickedStartThreadIii();
	afx_msg void OnBnClickedStopThreadIii();

	afx_msg void OnBnClickedStartThreadIv();
	afx_msg void OnBnClickedStopThreadIv();

	afx_msg void OnBnClickedStressStartTest();
	afx_msg void OnBnClickedStressStopTest();

	// ����PushMessage����
	CButton m_btn_StartStressTest;
	CButton m_btn_EndStressTest;
private:
	// TokenIDǰ׺
	CString m_strPreTokenID;
	// TokenID ��ʼ����
	UINT m_nTokenID_StartNum;
	UINT m_nTokenIDNum;

	UINT m_nCountPushMSg;						// ����PushMessage�ɹ�������
	LONG volatile m_nCountPushMsgPerMin;				// ÿ���ӷ���PushMsg��;
	DWORD m_dwStartPushMsgTime;
//	DWORD m_dwSpendPushMsgTime;
	CCriticalSection m_csAtomicOperation;

public:
	// PushMessage������ʱ��
	CEdit m_SpendTime;
	CEdit m_edit_MsgCount;
public:
	afx_msg void OnBnClickedStressPauseTest();
	// PushMessage ÿ���ӷ��ĸ���
	UINT m_nPushMsgRateNum;
};
