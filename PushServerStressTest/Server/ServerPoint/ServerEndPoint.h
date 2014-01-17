#pragma once
#include "stdafx.h"
#include <WinSock2.h>
#include "json.h"
#include "MD5.h"
#include "ServerDB.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

class CServerEndPoint;

//// ���ڷ������ݵ��̲߳���
//typedef struct _tagThreadParams_WORKER
//{
//	CServerEndPoint* pClient;                       // ��ָ�룬���ڵ������еĺ���
//	SOCKET   sock;										// ÿ���߳�ʹ�õ�Socket
//	int      nThreadNo;									// �̱߳��
//	char     szBuffer[1024];
//
//} THREADPARAMS_WORKER,*PTHREADPARAM_WORKER;  
//
//// ����Socket���ӵ��߳�
//typedef struct _tagThreadParams_CONNECTION
//{
//	CServerEndPoint* pServerEndPointt;                         // ��ָ�룬���ڵ������еĺ���
//
//} THREADPARAMS_CONNECTION,*PTHREADPARAM_CONNECTION; 


class IServerEndPointNotify
{
public:
	virtual void OnSuccessSend(string strMsg)= 0;
	virtual void OnErrorSend(string strMsg) = 0;
	virtual void OnSuccessRecv(string strMsg) = 0;
};

class CServerEndPoint
{
public:
	CServerEndPoint(void);
	virtual ~CServerEndPoint(void);

public:
	//// ����SchoolID��SchoolKey
	//void SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey);

	// ����SchoolID��SchoolKey
	void SetAppInfo(const string& strAppID, const string& strAppKey);

	// ����TokenID
	void SetTokenID(const string& strTokenID);
	// ���ӵ�Push������
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	// ����Challenge
	BOOL GetChallenge();
	// ע��Server(��½Push������)
	BOOL LoginServer();
	// ���ʹ����CheckCode
//	BOOL SendErrorCheckCode2PushSRV();
	// ����Push��Ϣ
	BOOL SendPushMessage(/*string strTokenID, */string strBody);
	// ���¼�������
	BOOL NotifyPushSRVReloadData();
	// ���ó�ʱ�ĺ���(�������ͳ�ʱ�ͽ��ճ�ʱ)��nTimeOutΪ��ʱʱ��(��λ������)
	BOOL SetTimeOut(UINT nTimeOut);
	// �Ͽ�Push������������
	BOOL DisConnect2PushSRV();

	// ���ûص�
	BOOL AddListener(IServerEndPointNotify *ptr);
	// ȡ���ص�
	BOOL RemoveListener(IServerEndPointNotify *ptr);

private:

	// ���ڽ������ӵ��߳�
	static DWORD WINAPI _ConnectionThread(LPVOID lParam);
	// ���������յ�����Push������������
	static DWORD WINAPI _RecvThread(LPVOID lParam);
	// ��Push��������������
	static DWORD WINAPI _SendThread(LPVOID lParam);
	// ������յ�����Ϣ
	int RecvDataProc(LPCSTR pszRecvBuf);
	// ȥ��Httpͷ��ʣBody
	string RemoveHttpHead(LPCSTR pszRecvBuf);

private:
	//�׽��־��
	SOCKET m_socket;
	//���ӱ�־
	BOOL m_bIsConnected;
	//��ʱ��־
	BOOL m_bIsTimeOut;
	//Push������IP�Ͷ˿�
	SOCKADDR_IN m_LocalServerAddr;
	//���ط�����IP�Ͷ˿�
	SOCKADDR_IN m_PushServerAddr;

	string m_strChallenge;

	//string m_strSchoolID;					// SchoolID
	//string m_strSchoolKey;				// SchoolKey
	string m_strAppID;						// AppID
	string m_strAppKey;					// AppKey
	string m_strTokenID;					// TokenID
	string m_strSendContent;			// Ҫ���͵�����

	IServerEndPointNotify *m_pListener;
};
