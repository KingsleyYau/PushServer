#pragma once
#include "stdafx.h"
#include "json.h"
#include "MD5.h"
#include "LogFile.h"
#include "Type.h"
#include "ClientDB.h"
#include <list>

//======�����ش�����庬��=======//
#define FAIL_CONNECT_PUSHSRV 0x00000001				// ����PushServerʧ��
#define FAIL_GET_CHALLENGE		  0x00000002				// getchallengeʧ��
#define FAIL_REGISTER_TOKENID 0x00000003				// ע��tokenidʧ��
#define FAIL_GET_PUSHMESSAGE 0x00000004				// getpushmessageʧ��	
#define FAIL_SET_PUSHMESSAGE  0x00000005				// setpushmessageʧ��
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

// ��Ա�������ⲿ�ӿڣ�
	
	// ����TokenID
	void SetTokenID(const string& strTokenID);
	// ����SchoolID��SchoolKey
	void SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey);
	// ����Push������IP�Ͷ˿�
	void SetPushSRVAddr(DWORD dwServerIP, UINT nPort);
	// ���ӵ�Push������
	BOOL Connect2PushSRV(DWORD dwServerIP, UINT nPort);
	BOOL Connect2PushSRV();
	// ����Challenge
	BOOL GetChallenge();
	// ע��TokenID
	BOOL RegisterTokenID();
	// ���ʹ����CheckCode
	BOOL SendErrorCheckCode2PushSRV();
	// ����Push��Ϣ
	BOOL GetPushMessage();
	// �Ͽ�Push������������
	BOOL DisConnect2PushSRV();

//=====ѹ�����Ե��ýӿ�======//
	// ��ʼ�������ݽ��ղ���
	int StartRecvDataTest();
	// ֹͣ���ݽ��ղ���
	int StopRecvDataTest();

//=====ѹ�����Ե��ýӿ�======//

private:
	// ���ó�ʱ
	BOOL SetTimeOut(UINT nTimeOut);
	// ȥ��Httpͷ��ʣBody
	string RemoveHttpHead(LPCSTR pszRecvBuf);

	// ��������
	BOOL ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList);
	// �������ݿ�
	//BOOL InsertPushMsg2DB()
	
	/* ����StartRecvDataTest�󣬿�һ���̣߳����ڽ���
	* PushServer�����������ݣ���д�뵽���ݿ� */
	static DWORD WINAPI RecvDataThread(LPVOID lParam);

// ��Ա����
private:

	// �׽��־��
	SOCKET m_hsocket;
	// ���ӱ�־
	BOOL m_bIsConnected;
	// ��ʱ��־
	BOOL m_bIsTimeOut;
	// Push������IP�Ͷ˿�
	SOCKADDR_IN m_PushServerAddr;

	// Push���������ص�challenge
	string m_strChallenge;
	// TokenID
	string m_strTokenID;
	// SchoolID
	string m_strSchoolID;
	// SchoolKey
	string m_strSchoolKey;

	// Recv�յ���Buffer�����ڽ������ݣ�
	string m_strRecvBuffer;
	
	// �߳̾��
	HANDLE m_hTreadRecvData;
	// �߳��Ƿ���Ҫ�ж�
	BOOL m_bIsThreadshouldAbort;
	
};
