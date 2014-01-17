#pragma once
#include "afxwin.h"
#include "ClientEndPoint.h"
#include "MsgDefine.h"


// CStressTestDlg �Ի���

class CStressTestDlg : public CDialog
{
	DECLARE_DYNAMIC(CStressTestDlg)

public:
	CStressTestDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CStressTestDlg();

// �Ի�������
	enum { IDD = IDD_STRESS_DLG };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

	// �̴߳�����
	static DWORD WINAPI ThreadProc(LPVOID lParam);
//public:
private:
	// �����߳���
	UINT m_nThreadNum;


	afx_msg void OnBnClickedBtnStartTest();
	afx_msg void OnBnClickedBtnEndTest();
	// �����߳�
	CButton m_btn_StartTest;
	// �����߳�
	CButton m_btn_EndTest;
	// �߳̾��
	HANDLE m_hThread;
	// �߳��Ƿ�����
	BOOL m_bIsThreadRun;

	// ֹͣ�¼�
	HANDLE m_hEventStopThread;

	// TokenIDǰ׺
	CString m_strPreTokenID;
	// TokenID ��ʼ����
	UINT m_TokenID_StartNum;
};
