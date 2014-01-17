#pragma once

#include "LocalDBManager.h"
class ClientDB :
	public LocalDBManager
{
public:
	ClientDB(void);
	virtual ~ClientDB(void);
	static ClientDB* GetInstance();
	BOOL InsertPushMsg2DB(string strTokenID, string strBody);

protected:
	BOOL CreateAllDB(BOOL& bResult);						//创建或打开所有的数据库
	BOOL CreatePushMessageDB();
	BOOL IsPushMsgDBExist();

private:
	CRITICAL_SECTION m_csInsertData;							// 将数据插入数据库时需将其设成原子操作
	

};
