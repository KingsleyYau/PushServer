#pragma once
#include "ClientEndPoint.h"
#include "MsgDefine.h"

// CRouTineTest �Ի���

class CRouTineTest : public CDialog
{
	DECLARE_DYNAMIC(CRouTineTest)

public:
	CRouTineTest(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CRouTineTest();

// �������ڵ���

	void SetServerIP(DWORD dwServerIP){m_dwPushServerIP = dwServerIP;}
	void SetServerPort(UINT nServerPort){m_nPushServerPort = nServerPort;}
	void SetTimeOut(UINT nTimeSlot){m_nTimeslot = nTimeSlot;}

// �Ի�������
	enum { IDD = IDD_ROUTINE_TEST};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedStartThreadI();
	afx_msg void OnBnClickedStopThreadI();

	afx_msg void OnBnClickedStartThreadIi();
	afx_msg void OnBnClickedStopThreadIi();

	afx_msg void OnBnClickedStartThreadIii();
	afx_msg void OnBnClickedStopThreadIii();

	afx_msg void OnBnClickedStartThreadIv();
	afx_msg void OnBnClickedStopThreadIv();

	DECLARE_MESSAGE_MAP()

	// �̴߳�����
	static DWORD WINAPI Thread_I_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_II_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_III_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_IV_Proc(LPVOID lParam);

private:
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

	// �ĸ������߳̾��
	HANDLE m_hThreadTest_I;
	HANDLE m_hThreadTest_II;
	HANDLE m_hThreadTest_III;
	HANDLE m_hThreadTest_IV;

	// ��־�ĸ��߳��Ƿ�����
	BOOL m_bIsThread_I_Run;
	BOOL m_bIsThread_II_Run;
	BOOL m_bIsThread_III_Run;
	BOOL m_bIsThread_IV_Run;

	UINT m_nTimeslot;
	DWORD m_dwPushServerIP;
	UINT m_nPushServerPort;
	
};
