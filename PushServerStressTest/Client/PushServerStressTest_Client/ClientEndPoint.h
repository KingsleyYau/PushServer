#pragma once
#include "stdafx.h"
#include "json.h"
#include "MD5.h"
#include "LogFile.h"
#include "Type.h"
#include "ClientDB.h"
#include <list>

//======出错返回代码具体含义=======//
#define FAIL_CONNECT_PUSHSRV 0x00000001				// 连接PushServer失败
#define FAIL_GET_CHALLENGE		  0x00000002				// getchallenge失败
#define FAIL_REGISTER_TOKENID 0x00000003				// 注册tokenid失败
#define FAIL_GET_PUSHMESSAGE 0x00000004				// getpushmessage失败	
#define FAIL_SET_PUSHMESSAGE  0x00000005				// setpushmessage失败
#define SOCKET_CLOSE				0x00000020

typedef struct _tagPushMesage 
{
	string strTokenID;
	string strBody;
}PUSHMESSAGE,*PPUSHMESSAGE;

typedef list<PUSHMESSAGE> PushMsgList;

using namespace std;

#pragma comment(lib, "ws2_32.lib")

class CClientEndPoint
{
public:
	CClientEndPoint(void);
	~CClientEndPoint(void);

// 成员函数（外部接口）
	
	// 设置TokenID
	void SetTokenID(const string& strTokenID);
	// 设置SchoolID和SchoolKey
	void SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey);
	// 设置Push服务器IP和端口
	void SetPushSRVAddr(DWORD dwServerIP, UINT nPort);
	// 连接到Push服务器
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	BOOL Connect2PushSRV();
	// 请求Challenge
	BOOL GetChallenge();
	// 注册TokenID
	BOOL RegisterTokenID();
	// 发送错误的CheckCode
	BOOL SendErrorCheckCode2PushSRV();
	// 请求Push消息
	BOOL GetPushMessage();
	// 断开Push服务器的连接
	BOOL DisConnect2PushSRV();

//=====压力测试调用接口======//
	// 开始进行数据接收测试
	int StartRecvDataTest();
	// 停止数据接收测试
	int StopRecvDataTest();

//=====压力测试调用接口======//

private:
	// 设置超时
	BOOL SetTimeOut(UINT nTimeOut);
	// 去除Http头，剩Body
	string RemoveHttpHead(LPCSTR pszRecvBuf);

	// 解析数据
	BOOL ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList);
	// 插入数据库
	//BOOL InsertPushMsg2DB()
	
	/* 调用StartRecvDataTest后，开一个线程，用于接收
	* PushServer发过来的数据，并写入到数据库 */
	static DWORD WINAPI RecvDataThread(LPVOID lParam);

// 成员变量
private:

	// 套接字句柄
	SOCKET m_hsocket;
	// 连接标志
	BOOL m_bIsConnected;
	// 超时标志
	BOOL m_bIsTimeOut;
	// Push服务器IP和端口
	SOCKADDR_IN m_PushServerAddr;

	// Push服务器返回的challenge
	string m_strChallenge;
	// TokenID
	string m_strTokenID;
	// SchoolID
	string m_strSchoolID;
	// SchoolKey
	string m_strSchoolKey;

	// Recv收到的Buffer（用于解析数据）
	string m_strRecvBuffer;
	
	// 线程句柄
	HANDLE m_hTreadRecvData;
	// 线程是否需要中断
	BOOL m_bIsThreadshouldAbort;
	
};
