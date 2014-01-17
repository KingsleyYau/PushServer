// PushServerStressTest_ClientDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "RouTineTest.h"
#include "StressTestDlg.h"


// CPushServerStressTest_ClientDlg 对话框
class CPushServerStressTest_ClientDlg : public CDialog
{
// 构造
public:
	CPushServerStressTest_ClientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_PUSHSERVERSTRESSTEST_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnGetPushServerIP(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetPushServerPort(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

//	子窗口类
private:
	CTabCtrl m_TabCtrl;		
	CRouTineTest m_dlgRouTineTest;
	CStressTestDlg m_dlgStressTest;
	

public:
	afx_msg void OnTcnSelchangeTabTest(NMHDR *pNMHDR, LRESULT *pResult);
private:
	DWORD m_dwServerIP;
};
