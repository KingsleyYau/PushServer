// PushServerStressTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <WinSock2.h>
#include "ServerEndPoint.h"
#include "afxcmn.h"
#include <afxmt.h>
// CPushServerStressTestDlg 对话框
class CPushServerStressTestDlg : public CDialog
{
// 构造
public:
	CPushServerStressTestDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CPushServerStressTestDlg();

// 对话框数据
	enum { IDD = IDD_PUSHSERVERSTRESSTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
//	afx_msg void OnStartTimer();
//	afx_msg void OnStopTimer();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()


// 线程处理函数
	
	static DWORD WINAPI Thread_I_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_II_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_III_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_IV_Proc(LPVOID lParam);
	static DWORD WINAPI Thread_Stress_Test(LPVOID lParam);


//成员变量
private:
	//参数设置
	DWORD m_dwServerIP;			// Push服务器IP
	UINT m_nServerPort;				// Push服务器端口
	CString m_strSendContent;		// 向Push服务器发送的内容

	//控件关联变量

	CDialog *m_TabPage;
	CEdit m_SendContent;		


	// 四个测试线程句柄

	HANDLE m_hThreadTest_I;
	HANDLE m_hThreadTest_II;
	HANDLE m_hThreadTest_III;
	HANDLE m_hThreadTest_IV;

	// Stress Test(循环发PushMessage线程)
	HANDLE m_hThreadStressTest;
	HANDLE m_hEventPauseTest;
	HANDLE m_hEventMinuteLock;
	// 标志四个线程是否在跑
	BOOL m_bIsThread_I_Run;
	BOOL m_bIsThread_II_Run;
	BOOL m_bIsThread_III_Run;
	BOOL m_bIsThread_IV_Run;
	
	BOOL m_bIsThread_Test_Run;

	//CServerEndPoint SRVEndPoint;

	//  测试的时间间隔

	UINT m_nTimeslot;

public:
	//afx_msg void OnBnClickedConn();

	//CTabCtrl m_TabCtrl;

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

	// 启动PushMessage测试
	CButton m_btn_StartStressTest;
	CButton m_btn_EndStressTest;
private:
	// TokenID前缀
	CString m_strPreTokenID;
	// TokenID 起始号码
	UINT m_nTokenID_StartNum;
	UINT m_nTokenIDNum;

	UINT m_nCountPushMSg;						// 发送PushMessage成功的数量
	LONG volatile m_nCountPushMsgPerMin;				// 每分钟发的PushMsg数;
	DWORD m_dwStartPushMsgTime;
//	DWORD m_dwSpendPushMsgTime;
	CCriticalSection m_csAtomicOperation;

public:
	// PushMessage经过的时间
	CEdit m_SpendTime;
	CEdit m_edit_MsgCount;
public:
	afx_msg void OnBnClickedStressPauseTest();
	// PushMessage 每分钟发的个数
	UINT m_nPushMsgRateNum;
};
