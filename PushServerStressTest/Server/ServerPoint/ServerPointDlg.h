// ServerPointDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "ServerEndPoint.h"
#include "afxmt.h"

#define SUCCESS_CONNECT WM_USER +0x101
#define ERROR_CONNECT SUCCESS_CONNECT +1
#define  SHOW_MSG2DLG ERROR_CONNECT+1

// CServerPointDlg 对话框
class CServerPointDlg : public CDialog, public IServerEndPointNotify
{
// 构造
public:
	CServerPointDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SERVERPOINT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持



// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
//	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnSuccessConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnErrorConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSHowMSG2Dlg(WPARAM wParam, LPARAM lParam);


	// IServerEndPointNotify接口
	void OnSuccessSend(string strMsg);
	void OnErrorSend(string strMsg);
	void OnSuccessRecv(string strMsg);
private:
	
	DWORD dwPushSerIP;		// Push服务器IP
	UINT nPort;						// PushServer端口
	CString m_strApplID;
	CString m_strAppKey;
	CString m_strTokenID;
	int m_nSend_Interval;			// 发送间隔（单位：秒）

	CString m_strSource;			// 源字符串
	CString m_strMD5;				// MD5字符串

	CButton m_btn_Connect;				// 连接
	CButton m_btn_SendContent;		// 发内容
	CButton m_btn_ClearText;			// 清除内容
	CButton m_btn_CycleSend;			// 循环发送按钮
	CButton m_btn_Stop_CycleSend;	// 停止循环发送

	// 要发送的内容
	CEdit m_edt_Content;
	CEdit m_Edt_MsgRecord;
	CString m_strContent;

	CString m_strMsg_Record;			// 显示已发送及收到的内容

	afx_msg void OnBnClickedLoginServer();

	// 线程处理函数
	static DWORD WINAPI _ConnectThreadProc(LPVOID lParam);
	static DWORD WINAPI _SendDataThreadProc(LPVOID lParam);
	static DWORD WINAPI _CycleSendPushMsgThreadProc(LPVOID lParam);

	HANDLE m_hThreadConn;
	HANDLE m_hThreadSend;
	HANDLE m_hThreadCycleSend;
	BOOL  m_bStopCycleSend;
	//CEvent	m_CycleSendInterval_event;

	CServerEndPoint m_SrvPoint;
	BOOL m_bConnState;							// 当前CServerEndPoint 的连接状态
	DWORD m_nConnThreadID;
	DWORD m_nCycleSendThreadID;
	afx_msg void OnBnClickedGenerateMd5();			// 生成MD5
	afx_msg void OnBnClickedSend();						// 发送
	afx_msg void OnBnClickedClearText();					// 清空
	afx_msg void OnBnClickedCycleSend();				// 循环发送
	afx_msg void OnBnClickedStopCycleSend();			// 停止循环发送

};
