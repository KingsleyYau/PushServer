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
	BOOL CreateAllDB(BOOL& bResult);						//����������е����ݿ�
	BOOL CreatePushMessageDB();
	BOOL IsPushMsgDBExist();

private:
	CRITICAL_SECTION m_csInsertData;							// �����ݲ������ݿ�ʱ�轫�����ԭ�Ӳ���
	

};
