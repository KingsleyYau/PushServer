#include "StdAfx.h"
#include "ClientDB.h"

// 创建PushMsg数据库，存放Client收到的PushMessage
LPCTSTR	lpszCreatePushMsgDB = _T("CREATE TABLE Client_PushMsg_db (\
														[TokenID] nvarchar(64) NOT NULL\
														,[Body] nvarchar(1024) NOT NULL\
														,PRIMARY KEY (TokenID,Body) \
														)");
// 查看数据库
LPCTSTR lpszSelectPushMsgDB = __T("SELECT * FROM Client_PushMsg_db");
// 插入数据
LPCTSTR lpszInsertPushMsg2DB = _T("INSERT INTO Client_PushMsg_db (TokenID,Body) VALUES('%s', '%s')");

ClientDB::ClientDB(void)
{
	m_UpdateVerNum = MAKEWORD(1,0);
	::InitializeCriticalSection(&m_csInsertData);
}

ClientDB::~ClientDB(void)
{
	::DeleteCriticalSection(&m_csInsertData);
}

ClientDB* ClientDB::GetInstance()
{
	static ClientDB g_ClientDB;
	return &g_ClientDB;
}

// 把TokenID 和Body插入数据库
BOOL ClientDB::InsertPushMsg2DB(string strTokenID, string strBody)
{
	EnterCriticalSection(&m_csInsertData);
	BOOL bRet = FALSE;
	TCHAR szSql[1024] = {0};
	_stprintf_s(szSql, lpszInsertPushMsg2DB, S2T(strTokenID).c_str(), S2T(strBody).c_str());
	bRet =  ClientDB::GetInstance()->ExecuteSQL(szSql);
	LeaveCriticalSection(&m_csInsertData);
	return bRet;
}

BOOL ClientDB::CreatePushMessageDB()
{

	BOOL bResult = FALSE;
	// 判断是否已存在
	bResult = IsPushMsgDBExist();
	// 若存在，则不建表
	bResult = bResult || ClientDB::GetInstance()->ExecuteSQL(lpszCreatePushMsgDB);
	return bResult;
}

BOOL ClientDB::IsPushMsgDBExist()
{
	return GetInstance()->ExecuteSQL( lpszSelectPushMsgDB, NULL);
}

BOOL ClientDB::CreateAllDB(BOOL& bResult)
{
	bResult = bResult && CreateVersionDB();

	bResult = bResult && CreatePushMessageDB();

	return bResult;
}