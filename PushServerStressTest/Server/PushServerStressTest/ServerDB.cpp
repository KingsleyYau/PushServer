#include "StdAfx.h"
#include "ServerDB.h"

// ����PushMsg���ݿ⣬���Client�յ���PushMessage
LPCTSTR	lpszCreatePushMsgDB = _T("CREATE TABLE Server_PushMsg_db (\
								 [TokenID] nvarchar(64) NOT NULL\
								 ,[Body] nvarchar(1024) NOT NULL\
								 ,PRIMARY KEY (TokenID,Body) \
								 )");

// �鿴���ݿ�
LPCTSTR lpszSelectPushMsgDB = __T("SELECT * FROM Server_PushMsg_db");
// ��������
LPCTSTR lpszInsertPushMsg2DB = _T("INSERT INTO Server_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");


ServerDB::ServerDB(void)
{
	m_UpdateVerNum = MAKEWORD(1,0);
	::InitializeCriticalSection(&m_csInsertData);
}

ServerDB::~ServerDB(void)
{
	::DeleteCriticalSection(&m_csInsertData);
}

ServerDB* ServerDB::GetInstance()
{
	static ServerDB g_ServerDB;
	return &g_ServerDB;
}


// ��TokenID ��Body�������ݿ�
BOOL ServerDB::InsertPushMsg2DB(string strTokenID, string strBody)
{
	EnterCriticalSection(&m_csInsertData);
	BOOL bRet = FALSE;
	TCHAR szSql[1024] = {0};
	_stprintf_s(szSql, lpszInsertPushMsg2DB, S2T(strTokenID).c_str(), S2T(strBody).c_str());
	bRet =  ServerDB::GetInstance()->ExecuteSQL(szSql);
	LeaveCriticalSection(&m_csInsertData);
	return bRet;
}

BOOL ServerDB::CreatePushMessageDB()
{
	BOOL bResult = FALSE;
	// �ж��Ƿ��Ѵ���
	bResult = IsPushMsgDBExist();
	// �����ڣ��򲻽���
	bResult = bResult || ServerDB::GetInstance()->ExecuteSQL(lpszCreatePushMsgDB);
	return bResult;
}

BOOL ServerDB::IsPushMsgDBExist()
{
	return GetInstance()->ExecuteSQL(lpszSelectPushMsgDB, NULL);
}

BOOL ServerDB::CreateAllDB(BOOL& bResult)
{
	bResult = bResult && CreateVersionDB();

	bResult = bResult && CreatePushMessageDB();

	return bResult;
}