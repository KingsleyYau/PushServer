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
//=== 定义PushMessage消息结构体===//
typedef struct _tagPushMesage 
{
	string strTokenID;
	string strBody;
}PUSHMESSAGE,*PPUSHMESSAGE;
// PushMessage消息链表
typedef list<PUSHMESSAGE> PushMsgList;

//=========================//

enum status{login, online, logoff,offline};						// 当前对象的状态，在线或掉线
typedef struct _tagtimethreshold
{
	UINT nMinOfflineIntervalTime;				// 最小下线等待时间
	UINT nMaxOfflineIntervalTime;			// 最大下线等待时间

	UINT nMinOnlineIntervalTime;				// 最小上线等待时间
	UINT nMaxOnlineIntervalTime;			// 最大上线等待时间

	struct _tagtimethreshold()
	{
		nMinOfflineIntervalTime  = 5;			// 单位：（秒）
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

// 成员函数（外部接口）
	
	// 设置TokenID
	void SetTokenID(const string& strTokenID);
	// 设置SchoolID和SchoolKey
	void SetSchoolInfo(const string& strSchoolID,  string strSchoolKey);
	// 设置Push服务器IP和端口
	void SetPushSRVAddr(DWORD dwServerIP, UINT nPort);
	// 设置上线和下线的时间阀值
	void SetTimeThreshold(const TIME_THRESHOLD& Timehreshold);
	// 连接到Push服务器
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	BOOL Connect2PushSRV();
	// 断开Push服务器的连接
	BOOL DisConnect2PushSRV();
	// 停止向PushServer发送消息
	BOOL StopSendMsg2PushSRV();			

	BOOL SwithStatus2Online();				// 上线（连接到PushServer）
	BOOL SwithStatus2Offline();				// 下线（断开连接）

	// 返回SOCKET句柄
	HANDLE GetSocketHandle()
	{
		return (HANDLE)m_hsocket;
	}

	// 
	DWORD AutoDecideStatus(DWORD nMilliSec);

	// 根据nOpType，选择下一步的操作
	DWORD SeleteOperation(LPOVERLAPPED lpOverlapped, DWORD dwIOBytesSize);		


//=========与PushServer 交互过程=========================//
	// WsaSend GetChallenge
	BOOL AsyncSendGetChallenge();		// 第一步操作，由外部调用，接下来的操作由内部调

private:
	// WsaSend Getchallenge返回，判断是否成功，成功则进行下一步操作
	void OnSendGetChallengeRet(DWORD dwIOBytesSize);
	// WsaRecv GetChallenge
	BOOL AsyncRecvGetChallenge();
	// WsaRecv Getchallenge返回，判断是否成功，或成功则进行下一步操作
	void OnRecvGetChallengeRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);
	
	// WsaSend RegisterTokenID
	BOOL AsyncSendRegisterTokenID();
	// WsaSend RegisterTokenID返回，判断是否成功，成功则进行下一步操作
	void OnSendRegisterTokenIDRet(DWORD dwIOBytesSize);
	// WsaRecv RegisterTokenID
	BOOL AsyncRecvRegisterTokenID();
	// WsaRecv RegisterTokenID返回，判断是否成功，成功则进行下一步操作
	void OnRecvRegisterTokenIDRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

	// AsyncSend PushMessage
	BOOL AsyncSendPushMessage();
	// WsaSend PushMessage返回
	void OnSendPushMessageRet(DWORD dwIOBytesSize);
	// WsaRecv PushMessage
	BOOL AsyncRecvPushMessage();
	// WsaRecv PushMessage返回
	void OnRecvPushMessageRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

	// 不断接收PushMessage
	void RecvPushMessage(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize);

//=====================================================//

	// 去除Http头，剩Body
	string RemoveHttpHead(LPCSTR pszRecvBuf);
	// 解析数据
	BOOL ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList);
	// 设置超时
	BOOL SetTimeOut(UINT nTimeOut);
private:
//////////////////////////////////////////////////////////////////////////
	
	SOCKET m_hsocket;							// 套接字句柄
	SOCKADDR_IN m_PushServerAddr;		// Push服务器IP和端口
	string m_strChallenge;						// Push服务器返回的challenge
	string m_strTokenID;							// TokenID
	string m_strSchoolID;							// SchoolID
	string m_strSchoolKey;						// SchoolKey
	
	string m_strRecvBuffer;						// Recv收到的Buffer（用于解析数据）
	IO_OPERATION_DATA m_IO_Send;
	IO_OPERATION_DATA m_IO_Recv;

	status m_CurrentStatus;						// 当前状态
	TIME_THRESHOLD m_TimeShreshold;	// 存储两个时间（上线时间和下线时间的阀值）
	UINT m_nTime2SwithStatus;
	UINT m_nCurrentTime;

	// 连接标志
	BOOL m_bIsConnected;
	// 超时标志
	BOOL m_bIsTimeOut;
};
