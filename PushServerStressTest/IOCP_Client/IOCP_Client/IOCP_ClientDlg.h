// IOCP_ClientDlg.h : 头文件
//

#pragma once
#include "IOCPModel.h"


// CIOCP_ClientDlg 对话框
class CIOCP_ClientDlg : public CDialog
{
// 构造
public:
	CIOCP_ClientDlg(CWnd* pParent = NULL);	// 标准构造函数
	

// 对话框数据
	enum { IDD = IDD_IOCP_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStartTest();
	afx_msg void OnBnClickedStopTest();
private:
	// 客户端数（并发数）
	UINT m_nClientNum;
	// 服务器端口
	UINT m_nServerPort;
	// TokenID 前缀
	CString m_strPreTokenID;
	// PushServerIP
	DWORD m_dwServerIP;
	// TokenID 起始号
	UINT m_TokenID_StartNum;
	CIOCPModel m_IOCPModel;
	// 最小上线等待时间
	UINT m_nMinOnlineTime;
	// 最大上线等待时间	
	UINT m_nMaxOnlineTime;
	// 最小下线等待时间
	UINT m_nMinOfflineTime;
	// 最大下线等待时间
	UINT m_nMaxOfflineTime;
};
