#pragma once
#include <WinSock2.h>
#include "json.h"
//#include "..\DataProcessing\include\MD5.h"
#include "MD5.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

class CServerEndPoint;

// 用于发送数据的线程参数
typedef struct _tagThreadParams_WORKER
{
	CServerEndPoint* pClient;                       // 类指针，用于调用类中的函数
	SOCKET   sock;										// 每个线程使用的Socket
	int      nThreadNo;									// 线程编号
	char     szBuffer[1024];

} THREADPARAMS_WORKER,*PTHREADPARAM_WORKER;  

// 产生Socket连接的线程
typedef struct _tagThreadParams_CONNECTION
{
	CServerEndPoint* pServerEndPointt;                         // 类指针，用于调用类中的函数

} THREADPARAMS_CONNECTION,*PTHREADPARAM_CONNECTION; 

class CServerEndPoint
{
public:
	CServerEndPoint(void);
	virtual ~CServerEndPoint(void);

public:
	// 设置SchoolID和SchoolKey
	void SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey);
	// 连接到Push服务器
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	// 请求Challenge
	BOOL GetChallenge();
	// 注册Server(登陆Push服务器)
	BOOL LoginServer();
	// 发送错误的CheckCode
	BOOL SendErrorCheckCode2PushSRV();
	// 发送Push消息
	BOOL SendPushMessage(CString strTokenIDlist, CString strBody);
	// 重新加载数据
	BOOL NotifyPushSRVReloadData();
	// 设置超时的函数(包括发送超时和接收超时)，nTimeOut为超时时间(单位：毫秒)
	BOOL SetTimeOut(UINT nTimeOut);
	// 断开Push服务器的连接
	BOOL DisConnect2PushSRV();

private:

	// 用于建立连接的线程
	static DWORD WINAPI _ConnectionThread(LPVOID lParam);
	// 用来处理收到来自Push服务器的数据
	static DWORD WINAPI _RecvThread(LPVOID lParam);
	// 往Push服务器发送数据
	static DWORD WINAPI _SendThread(LPVOID lParam);
	// 处理接收到的消息
	int RecvDataProc(LPCSTR pszRecvBuf);
	// 去除Http头，剩Body
	string RemoveHttpHead(LPCSTR pszRecvBuf);


private:
	//套接字句柄
	SOCKET m_socket;
	//连接标志
	BOOL m_bIsConnected;
	//超时标志
	BOOL m_bIsTimeOut;
	//Push服务器IP和端口
	SOCKADDR_IN m_LocalServerAddr;
	//本地服务器IP和端口
	SOCKADDR_IN m_PushServerAddr;

//	CString m_strLocalIP;

	string m_strChallenge;
	CString m_strTokenIDList;			//发送的TokenIDList
	string m_strSchoolID;				//SchoolID
	string m_strSchoolKey;				//SchoolKey
	
	

};
