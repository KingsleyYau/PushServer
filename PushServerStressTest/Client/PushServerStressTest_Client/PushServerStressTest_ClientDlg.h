// PushServerStressTest_ClientDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "RouTineTest.h"
#include "StressTestDlg.h"


// CPushServerStressTest_ClientDlg �Ի���
class CPushServerStressTest_ClientDlg : public CDialog
{
// ����
public:
	CPushServerStressTest_ClientDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_PUSHSERVERSTRESSTEST_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnGetPushServerIP(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetPushServerPort(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

//	�Ӵ�����
private:
	CTabCtrl m_TabCtrl;		
	CRouTineTest m_dlgRouTineTest;
	CStressTestDlg m_dlgStressTest;
	

public:
	afx_msg void OnTcnSelchangeTabTest(NMHDR *pNMHDR, LRESULT *pResult);
private:
	DWORD m_dwServerIP;
};
