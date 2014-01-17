// IOCP_ClientDlg.h : ͷ�ļ�
//

#pragma once
#include "IOCPModel.h"


// CIOCP_ClientDlg �Ի���
class CIOCP_ClientDlg : public CDialog
{
// ����
public:
	CIOCP_ClientDlg(CWnd* pParent = NULL);	// ��׼���캯��
	

// �Ի�������
	enum { IDD = IDD_IOCP_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStartTest();
	afx_msg void OnBnClickedStopTest();
private:
	// �ͻ���������������
	UINT m_nClientNum;
	// �������˿�
	UINT m_nServerPort;
	// TokenID ǰ׺
	CString m_strPreTokenID;
	// PushServerIP
	DWORD m_dwServerIP;
	// TokenID ��ʼ��
	UINT m_TokenID_StartNum;
	CIOCPModel m_IOCPModel;
	// ��С���ߵȴ�ʱ��
	UINT m_nMinOnlineTime;
	// ������ߵȴ�ʱ��	
	UINT m_nMaxOnlineTime;
	// ��С���ߵȴ�ʱ��
	UINT m_nMinOfflineTime;
	// ������ߵȴ�ʱ��
	UINT m_nMaxOfflineTime;
};
