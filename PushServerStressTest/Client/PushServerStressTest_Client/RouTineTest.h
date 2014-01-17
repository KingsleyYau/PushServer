#pragma once
#include "ClientEndPoint.h"
#include "MsgDefine.h"

// CRouTineTest 对话框

class CRouTineTest : public CDialog
{
	DECLARE_DYNAMIC(CRouTineTest)

public:
	CRouTineTest(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CRouTineTest();

// 供主窗口调用

	void SetServerIP(DWORD dwServerIP){m_dwPushServerIP = dwServerIP;}
	void SetServerPort(UINT nServerPort){m_nPushServerPort = nServerPort;}
	void SetTimeOut(UINT nTimeSlot){m_nTimeslot = nTimeSlot;}

// 对话框数据
	enum { IDD = IDD_ROUTINE_TEST};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
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

	// 线程处理函数
	static DWORD WINAPI Thread_I_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_II_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_III_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_IV_Proc(LPVOID lParam);

private:
	// 启动线程I
	CButton m_btnStartThread_I;
	// 停止线程1
	CButton m_btnStopThread_I;
	// 启动线程II
	CButton m_btnStartThread_II;
	// 停止线程II
	CButton m_btnStopThread_II;
	// 启动线程III
	CButton m_btnStartThread_III;
	// 停止线程III
	CButton m_btnStopThread_III;
	// 启动线程IV
	CButton m_btnStartThread_IV;
	// 停止线程IV
	CButton m_btnStopThread_IV;

	// 四个测试线程句柄
	HANDLE m_hThreadTest_I;
	HANDLE m_hThreadTest_II;
	HANDLE m_hThreadTest_III;
	HANDLE m_hThreadTest_IV;

	// 标志四个线程是否在跑
	BOOL m_bIsThread_I_Run;
	BOOL m_bIsThread_II_Run;
	BOOL m_bIsThread_III_Run;
	BOOL m_bIsThread_IV_Run;

	UINT m_nTimeslot;
	DWORD m_dwPushServerIP;
	UINT m_nPushServerPort;
	
};
