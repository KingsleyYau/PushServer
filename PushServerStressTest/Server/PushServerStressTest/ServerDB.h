#pragma once

#include "localdbmanager.h"
class ServerDB :
	public LocalDBManager
{
public:
	ServerDB(void);
	virtual ~ServerDB(void);
	static ServerDB* GetInstance();
	BOOL InsertPushMsg2DB(string strTokenID, string strBody);

protected:
	BOOL CreateAllDB(BOOL& bResult);						//创建或打开所有的数据库
	BOOL CreatePushMessageDB();
	BOOL IsPushMsgDBExist();

private:
	CRITICAL_SECTION m_csInsertData;							// 将数据插入数据库时需将其设成原子操作

};
