// MergeDBDlg.h : 头文件
//

#pragma once
#include "stdafx.h"
#include "CustomMessage.h"
#include <shlwapi.h>
#include "sqlite3.h"

// CMergeDBDlg 对话框
class CMergeDBDlg : public CDialog
{
// 构造
public:
	CMergeDBDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MERGEDB_DIALOG };

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
	afx_msg LRESULT OnMergeDBDone(WPARAM wParam, LPARAM lParam);					// 数据库合并已经完成
	afx_msg LRESULT OnDeleteSameRecDone(WPARAM wParam, LPARAM lParam);			// 删除相同记录已经完成
	DECLARE_MESSAGE_MAP()
	
	// 事件处函数
	afx_msg void OnBnClickedSelectSrcDir();
	afx_msg void OnBnClickedSelectDestDir();
	afx_msg void OnBnClickedMergeDb();
	afx_msg void OnBnClickedDeleteSameRec();

	// 搜索某个目录下所有的db文件
	BOOL SearchDBFile(LPCTSTR lpRootPath);

	// 数据库合并操作
	BOOL MergeDB();

private:
	static DWORD WINAPI _ThreadMergeDB(LPVOID lParam);					// 合并数据库
	static DWORD WINAPI _ThreadDeleteSameRecord(LPVOID lParam);		// 删除数据库

	// 浏览文件夹对话框回调
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam,	LPARAM lpData);

	static int ServerPushMsg_Callback(void* pvoid, int argc, char** argv, char** col);
	static int ClientPushMsg_Callback(void* pvoid, int argc, char** argv, char** col);


	string m_strDBSrcRootPath;
	string m_strDBDestPath;
	HANDLE m_hTreadMergeDB;
	HANDLE m_hTreadDeleteSameRecord;
	vector<string> m_DBPathVect;
	sqlite3 *m_pTargetDB;			// 目标数据库，把所有源数据库的记录都放到其中

	BOOL m_bIsDBMerging;			// 数据库是否在合并过程中
	BOOL m_bIsDBDeleting;			// 数据库是否在删除记录过程中
	//sqlite3 *m_pSourceDB;		


};
