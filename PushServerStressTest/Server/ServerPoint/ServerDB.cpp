#include "StdAfx.h"
#include "ServerDB.h"

// 创建PushMsg数据库，存放Client收到的PushMessage
LPCTSTR	lpszCreatePushMsgDB = _T("CREATE TABLE Server_PushMsg_db (\
								 [TokenID] nvarchar(64) NOT NULL\
								 ,[Body] nvarchar(1024) NOT NULL\
								 ,PRIMARY KEY (TokenID,Body) \
								 )");

// 查看数据库
LPCTSTR lpszSelectPushMsgDB = __T("SELECT * FROM Server_PushMsg_db");
// 插入数据
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


// 把TokenID 和Body插入数据库
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
	// 判断是否已存在
	bResult = IsPushMsgDBExist();
	// 若存在，则不建表
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