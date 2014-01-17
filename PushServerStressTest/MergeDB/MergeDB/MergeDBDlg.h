// MergeDBDlg.h : ͷ�ļ�
//

#pragma once
#include "stdafx.h"
#include "CustomMessage.h"
#include <shlwapi.h>
#include "sqlite3.h"

// CMergeDBDlg �Ի���
class CMergeDBDlg : public CDialog
{
// ����
public:
	CMergeDBDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MERGEDB_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnMergeDBDone(WPARAM wParam, LPARAM lParam);					// ���ݿ�ϲ��Ѿ����
	afx_msg LRESULT OnDeleteSameRecDone(WPARAM wParam, LPARAM lParam);			// ɾ����ͬ��¼�Ѿ����
	DECLARE_MESSAGE_MAP()
	
	// �¼�������
	afx_msg void OnBnClickedSelectSrcDir();
	afx_msg void OnBnClickedSelectDestDir();
	afx_msg void OnBnClickedMergeDb();
	afx_msg void OnBnClickedDeleteSameRec();

	// ����ĳ��Ŀ¼�����е�db�ļ�
	BOOL SearchDBFile(LPCTSTR lpRootPath);

	// ���ݿ�ϲ�����
	BOOL MergeDB();

private:
	static DWORD WINAPI _ThreadMergeDB(LPVOID lParam);					// �ϲ����ݿ�
	static DWORD WINAPI _ThreadDeleteSameRecord(LPVOID lParam);		// ɾ�����ݿ�

	// ����ļ��жԻ���ص�
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam,	LPARAM lpData);

	static int ServerPushMsg_Callback(void* pvoid, int argc, char** argv, char** col);
	static int ClientPushMsg_Callback(void* pvoid, int argc, char** argv, char** col);


	string m_strDBSrcRootPath;
	string m_strDBDestPath;
	HANDLE m_hTreadMergeDB;
	HANDLE m_hTreadDeleteSameRecord;
	vector<string> m_DBPathVect;
	sqlite3 *m_pTargetDB;			// Ŀ�����ݿ⣬������Դ���ݿ�ļ�¼���ŵ�����

	BOOL m_bIsDBMerging;			// ���ݿ��Ƿ��ںϲ�������
	BOOL m_bIsDBDeleting;			// ���ݿ��Ƿ���ɾ����¼������
	//sqlite3 *m_pSourceDB;		


};
