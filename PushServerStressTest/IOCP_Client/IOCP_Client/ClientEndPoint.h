#pragma once
#include "stdafx.h"
#include "json.h"
#include "MD5.h"
#include "LogFile.h"
#include "Type.h"
#include "ClientDB.h"
#include <list>
#include "afxmt.h"

extern LONG volatile g_nPushMsgNum; 	
#pragma comment(lib, "ws2_32.lib")

using namespace std;

class CClientEndPoint;

TSTRING GetErrCodeInfo(DWORD dwErrCode);
//=== ����PushMessage��Ϣ�ṹ��===//
typedef struct _tagPushMesage 
{
	string strTokenID;
	string strBody;
}PUSHMESSAGE,*PPUSHMESSAGE;
// PushMessage��Ϣ����
typedef list<PUSHMESSAGE> PushMsgList;

//=========================//

enum status{login, online, logoff,offline};						// ��ǰ�����״̬�����߻����
typedef struct _tagtimethreshold
{
	UINT nMinOfflineIntervalTime;				// ��С���ߵȴ�ʱ��
	UINT nMaxOfflineIntervalTime;			// ������ߵȴ�ʱ��

	UINT nMinOnlineIntervalTime;				// ��С���ߵȴ�ʱ��
	UINT nMaxOnlineIntervalTime;			// ������ߵȴ�ʱ��

	struct _tagtimethreshold()
	{
		nMinOfflineIntervalTime  = 5;			// ��λ�����룩
		nMaxOfflineIntervalTime = 10;
		nMinOnlineIntervalTime	 = 5;
		nMaxOnlineIntervalTime	 = 10;
	}
	struct _tagtimethreshold(const _tagtimethreshold &rhs)
	{
		nMinOfflineIntervalTime = rhs.nMinOfflineIntervalTime;
		nMaxOfflineIntervalTime = rhs.nMaxOfflineIntervalTime;
		nMinOnlineIntervalTime = rhs.nMinOnlineIntervalTime;
		nMaxOnlineIntervalTime = rhs.nMaxOnlineIntervalTime;
	}


}TIME_THRESHOLD, *PTIME_THRESHOLD;

class CClientEndPoint
{
public:
	CClientEndPoint(void);
	virtual ~CClientEndPoint(void);

// ��Ա�������ⲿ�ӿڣ�
	
	// ����TokenID
	void SetTokenID(const string& strTokenID);
	// ����SchoolID��SchoolKey
	void SetSchoolInfo(const string& strSchoolID,  string strSchoolKey);
	// ����Push������IP�Ͷ˿�
	void SetPushSRVAddr(DWORD dwServerIP, UINT nPort);
	// �������ߺ����ߵ�ʱ�䷧ֵ
	void SetTimeThreshold(const TIME_THRESHOLD& Timehreshold);
	// ���ӵ�Push������
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	BOOL Connect2PushSRV();
	// �Ͽ�Push������������
	BOOL DisConnect2PushSRV();
	// ֹͣ��PushServer������Ϣ
	BOOL StopSendMsg2PushSRV();			

	BOOL SwithStatus2Online();				// ���ߣ����ӵ�PushServer��
	BOOL SwithStatus2Offline();				// ���ߣ��Ͽ����ӣ�

	// ����SOCKET���
	HANDLE GetSocketHandle()
	{
		return (HANDLE)m_hsocket;
	}

	// 
	DWORD AutoDecideStatus(DWORD nMilliSec);

	// ����nOpType��ѡ����һ���Ĳ���
	DWORD SeleteOperation(LPOVERLAPPED lpOverlapped, DWORD dwIOBytesSize);		


//=========��PushServer ��������=========================//
	// WsaSend GetChallenge
	BOOL AsyncSendGetChallenge();		// ��һ�����������ⲿ���ã��������Ĳ������ڲ���

private:
	// WsaSend Getchallenge���أ��ж��Ƿ�ɹ����ɹ��������һ������
	void OnSendGetChallengeRet(DWORD dwIOBytesSize);
	// WsaRecv GetChallenge
	BOOL AsyncRecvGetChallenge();
	// WsaRecv Getchallenge���أ��ж��Ƿ�ɹ�����ɹ��������һ������
	void OnRecvGetChallengeRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);
	
	// WsaSend RegisterTokenID
	BOOL AsyncSendRegisterTokenID();
	// WsaSend RegisterTokenID���أ��ж��Ƿ�ɹ����ɹ��������һ������
	void OnSendRegisterTokenIDRet(DWORD dwIOBytesSize);
	// WsaRecv RegisterTokenID
	BOOL AsyncRecvRegisterTokenID();
	// WsaRecv RegisterTokenID���أ��ж��Ƿ�ɹ����ɹ��������һ������
	void OnRecvRegisterTokenIDRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

	// AsyncSend PushMessage
	BOOL AsyncSendPushMessage();
	// WsaSend PushMessage����
	void OnSendPushMessageRet(DWORD dwIOBytesSize);
	// WsaRecv PushMessage
	BOOL AsyncRecvPushMessage();
	// WsaRecv PushMessage����
	void OnRecvPushMessageRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

	// ���Ͻ���PushMessage
	void RecvPushMessage(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

//=====================================================//

	// ȥ��Httpͷ��ʣBody
	string RemoveHttpHead(LPCSTR pszRecvBuf);
	// ��������
	BOOL ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList);
	// ���ó�ʱ
	BOOL SetTimeOut(UINT nTimeOut);
private:
//////////////////////////////////////////////////////////////////////////
	
	SOCKET m_hsocket;							// �׽��־��
	SOCKADDR_IN m_PushServerAddr;		// Push������IP�Ͷ˿�
	string m_strChallenge;						// Push���������ص�challenge
	string m_strTokenID;							// TokenID
	string m_strSchoolID;							// SchoolID
	string m_strSchoolKey;						// SchoolKey
	
	string m_strRecvBuffer;						// Recv�յ���Buffer�����ڽ������ݣ�
	IO_OPERATION_DATA m_IO_Send;
	IO_OPERATION_DATA m_IO_Recv;

	status m_CurrentStatus;						// ��ǰ״̬
	TIME_THRESHOLD m_TimeShreshold;	// �洢����ʱ�䣨����ʱ�������ʱ��ķ�ֵ��
	UINT m_nTime2SwithStatus;
	UINT m_nCurrentTime;

	// ���ӱ�־
	BOOL m_bIsConnected;
	// ��ʱ��־
	BOOL m_bIsTimeOut;
};
