#pragma once
#include "afxwin.h"
#include "ClientEndPoint.h"
#include "MsgDefine.h"


// CStressTestDlg 对话框

class CStressTestDlg : public CDialog
{
	DECLARE_DYNAMIC(CStressTestDlg)

public:
	CStressTestDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CStressTestDlg();

// 对话框数据
	enum { IDD = IDD_STRESS_DLG };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

	// 线程处理函数
	static DWORD WINAPI ThreadProc(LPVOID lParam);
//public:
private:
	// 并发线程数
	UINT m_nThreadNum;


	afx_msg void OnBnClickedBtnStartTest();
	afx_msg void OnBnClickedBtnEndTest();
	// 开启线程
	CButton m_btn_StartTest;
	// 结束线程
	CButton m_btn_EndTest;
	// 线程句柄
	HANDLE m_hThread;
	// 线程是否运行
	BOOL m_bIsThreadRun;

	// 停止事件
	HANDLE m_hEventStopThread;

	// TokenID前缀
	CString m_strPreTokenID;
	// TokenID 起始号码
	UINT m_TokenID_StartNum;
};
