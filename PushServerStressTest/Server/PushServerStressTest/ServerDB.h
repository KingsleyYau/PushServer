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
	BOOL CreateAllDB(BOOL& bResult);						//����������е����ݿ�
	BOOL CreatePushMessageDB();
	BOOL IsPushMsgDBExist();

private:
	CRITICAL_SECTION m_csInsertData;							// �����ݲ������ݿ�ʱ�轫�����ԭ�Ӳ���

};
