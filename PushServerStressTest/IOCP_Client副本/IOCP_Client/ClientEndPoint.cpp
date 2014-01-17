#include "ClientEndPoint.h"
#include <Mstcpip.h>
#include "ClientQueue.h"

static int g_challengesuccess = 0;
static int g_registersuccess = 0;
static int g_pushmessagesuccess = 0;
LONG volatile g_nPushMsgNum = 0; 	

// ���ݴ�����뷵�ض�Ӧ�Ĵ�����Ϣ
TSTRING GetErrCodeInfo(DWORD dwErrCode)
{
	TSTRING strErrMessage;
	HLOCAL hlocal = NULL;			// Buffer that gets the error message string
	//DWORD dwLanguageId = MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED);	
	//DWORD dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	// ��ô����������������Ϣ
	BOOL fOk = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS|
		FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, dwErrCode, dwLanguageId,
		(PTSTR)&hlocal, 0, NULL);
	DWORD dwErr = GetLastError();
	if (!fOk) {
		// Is it a network-related error?
		HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, 
			DONT_RESOLVE_DLL_REFERENCES);

		if (hDll != NULL) {
			fOk = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
				FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL, dwErrCode, dwLanguageId,
				(PTSTR)&hlocal, 0, NULL);
			FreeLibrary(hDll);
		}
	}

	if (fOk && (hlocal != NULL)) 
	{
		strErrMessage = (PTSTR)hlocal;
		//strErrMessage += _T("\n");
		LocalFree(hlocal);
	} else {
		OutputDebugString(_T("��������Ҳ�����Ӧ����Ϣ\n"))	;
	}
	return strErrMessage;
}

UINT GenerateRandomTime(UINT nMin, UINT nMax)
{
	// ASSERT(nMax>= nMin && nMin >0);
	// 2012-12-20 Modified by Sanwen
	UINT randTime =0;
	if (0==nMin && 0==nMax)
	{
		randTime = (UINT)(-1)/1000;
	} 
	else
	{
		randTime = (((double) rand() / 
			(double) RAND_MAX) * (nMax-nMin)  + nMin);
	}
	return randTime;
}

LPCSTR HttpHead = "POST / HTTP/1.1\r\n"
"CHARSET: UTF-8\r\n"
"CONNECTION: KEEP-ALIVE\r\n"
"CONTENT-TYPE: TEXT/HTML\r\n"
"ACCEPT-ENCODING: GZIP\r\n";

CClientEndPoint::CClientEndPoint(void):
m_bIsConnected(FALSE),
m_bIsTimeOut(FALSE),
m_hsocket(INVALID_SOCKET),
m_CurrentStatus(offline)
{
	m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOnlineIntervalTime,
																m_TimeShreshold.nMaxOnlineIntervalTime);
	m_nCurrentTime = 0;
	m_strRecvBuffer.clear();
}

CClientEndPoint::~CClientEndPoint(void)
{
	if (INVALID_SOCKET != m_hsocket)
	{
		DisConnect2PushSRV();
	}
}

// ����TokenID
void  CClientEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	m_strTokenID = strTokenID;
}

// ����SchoolID��SchoolKey
void  CClientEndPoint::SetSchoolInfo(const string& strSchoolID, string strSchoolKey)
{
	ASSERT(!strSchoolID.empty());
	ASSERT(!strSchoolKey.empty());
	m_strSchoolID = strSchoolID;
	m_strSchoolKey = strSchoolKey;	
}
// ����PushServer��ַ��IP
void CClientEndPoint::SetPushSRVAddr(DWORD dwServerIP, UINT nPort)
{
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);
}

void CClientEndPoint::SetTimeThreshold(const TIME_THRESHOLD& Timehreshold)
{
	m_TimeShreshold = Timehreshold;
	m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOnlineIntervalTime,
		m_TimeShreshold.nMaxOnlineIntervalTime);
}

// ���ӵ�Push������
BOOL CClientEndPoint::Connect2PushSRV(DWORD dwServerIP, UINT nPort)
{
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);
	return Connect2PushSRV();
}

// ���ӵ�PushServer
BOOL CClientEndPoint::Connect2PushSRV()
{
	if (INVALID_SOCKET == m_hsocket)
	{
		//m_hsocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		m_hsocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
			NULL, 0, WSA_FLAG_OVERLAPPED);
		
		SOCKADDR_IN LocalAddr;
		LocalAddr.sin_family = AF_INET;
		LocalAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		LocalAddr.sin_port = htons(0);
		int nRet =bind(m_hsocket, (SOCKADDR*)&LocalAddr, sizeof(LocalAddr));
		if(nRet == SOCKET_ERROR)
		{
			OutputDebugString(_T("Socket ��ʧ��\n"));
			gvLog(LOG_WARNING, _T("Socket��ʧ��"));
			DisConnect2PushSRV();
			return FALSE;
		}

	} 
	// ����TCP Keep-Alive ����
	tcp_keepalive tcp_keepalive_param;
	tcp_keepalive_param.onoff = TRUE;
	tcp_keepalive_param.keepaliveinterval = 30000;			// ��ʮ�뷢��һ������
	tcp_keepalive_param.keepalivetime		= 60000;			// ��ʮ����û�յ�����Ϊ�����Ѿ��Ͽ�
	DWORD cbBytesReturned;
	if ( WSAIoctl(m_hsocket, SIO_KEEPALIVE_VALS,
		(LPVOID)&tcp_keepalive_param,
		sizeof(tcp_keepalive_param), 
		NULL, 0, (LPDWORD)&cbBytesReturned, NULL, NULL) == SOCKET_ERROR )
	{
		DWORD dwErrCode = WSAGetLastError();
		if (ERROR_IO_PENDING != dwErrCode )			// �Ƿ�ɹ������ص�����
		{
			OutputDebugString(_T("Socket ����Keep-aliveʧ��!\n"));
			gvLog(LOG_WARNING, _T("Socket ����Keep-aliveʧ��!\n") );
			return FALSE;
		}
	}

	// ��λ���ӱ�־
	m_bIsConnected = FALSE;
	// ��λ��ʱ��־
	m_bIsTimeOut = FALSE;

	if (0 != ::connect(m_hsocket, (SOCKADDR*)&(m_PushServerAddr),sizeof(m_PushServerAddr) ))
	{
		// ����ʧ��
		int nErr = WSAGetLastError();	
		TCHAR szBuf[32] = {0};
		_stprintf_s(szBuf,_T("���ӵ�������ʧ�ܣ�����ţ�%d"),nErr);
		OutputDebugString(szBuf);
		gvLog(LOG_WARNING, _T("Socket ���ӵ�PushServerʧ��!\n") );
		return FALSE;
	}
	else
	{
		m_CurrentStatus = login;
		m_nTime2SwithStatus = GenerateRandomTime(m_TimeShreshold.nMinOfflineIntervalTime,
																		m_TimeShreshold.nMaxOfflineIntervalTime);
		m_nCurrentTime = 0;
		m_bIsConnected = TRUE;
		// 2013-6-18   Added by Sanwen	
		if (SocketConnect == m_TestType)
		{
			DisConnect2PushSRV();
		}
		return TRUE;
	}	
}

// �Զ��л�״̬
DWORD CClientEndPoint::AutoDecideStatus(DWORD nMilliSec)
{
	m_nCurrentTime+= nMilliSec;
	switch (m_CurrentStatus)
	{
	case offline:
		{
			if (m_nCurrentTime >= m_nTime2SwithStatus)
			{
				// �����л�״̬��ʱ�䣬���Լ����뵽�����У��ȴ�Connect
				m_CurrentStatus = login;
				ClientConnQueue::GetInstance()->Push2Queue(this);
			}
		}
		break;
	case login:
		{
			if (m_nCurrentTime >60000)
			{
				// ���������֮��û�е�½�ϣ�����
				// Ϊ��½ʧ�ܣ�����Disconn���еȺ���
				//ClientDisConnQueue::GetInstance()->Push2Queue(this);
				DisConnect2PushSRV();
			}
		}
		break;
	case online:
		{
			if (m_nCurrentTime >= m_nTime2SwithStatus)
			{
				//m_CurrentStatus = logoff;
				//ClientDisConnQueue::GetInstance()->Push2Queue(this);
				StopSendMsg2PushSRV();
			}
		}
		break;
	case logoff:
		{
			m_nCurrentTime = 0;
		}
		break;
	}
	return 0;
}

BOOL CClientEndPoint::SwithStatus2Online()				// ���ߣ����ӵ�PushServer��
{
	m_nCurrentTime += 1000;
	if (m_CurrentStatus == online)
	{
		m_nTime2SwithStatus = GenerateRandomTime(m_TimeShreshold.nMinOfflineIntervalTime,
																			m_TimeShreshold.nMaxOnlineIntervalTime);
		m_nCurrentTime = 0;
		return TRUE;
	}
	else
	{
		// �ж�ʱ���Ƿ�����ʱ��
		if (m_nCurrentTime >= m_nTime2SwithStatus)
		{
			// �����л�ʱ��
			if (Connect2PushSRV())
			{
				// ���ӳɹ�
				return TRUE;
			} 
			else
			{
				// ����ʧ��
				DisConnect2PushSRV();
				return FALSE;
			}
		}
		else
		{
			// ��û�����л���ʱ��
			return FALSE;
		}
		// �����
		if (Connect2PushSRV())
		{
			// ���ӵ�PushServer�ɹ�
			AsyncSendGetChallenge();
			// ���¼�ʱ
			return TRUE;
		}
		else
		{
			DisConnect2PushSRV();
			return FALSE;
		}
		// ��
	}
	return TRUE;
}

BOOL CClientEndPoint::SwithStatus2Offline()				// ���ߣ��Ͽ����ӣ�
{
	if (m_CurrentStatus == offline)
	{
		return TRUE;
	} 
	else
	{
		// �ж��Ƿ�ʱ�䵽����ʱ��
		if (DisConnect2PushSRV())
		{
			// ���¼�ʱ
			return TRUE;
		}
		else
		{
			return FALSE;
		}

	}

	return TRUE;
}

// WsaSend GetChallenge
BOOL CClientEndPoint::AsyncSendGetChallenge()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;

	// ����Http Request Head
	strHttpHead = HttpHead;

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// ����Http Body
	Json::Value JsonParam;
	Json::FastWriter Fast_Writer;
	JsonParam["cmd"] = Json::Value("getclientchanllenge");
	JsonParam["TokenID"] = Json::Value(m_strTokenID);
	JsonParam["SocketHandle"] = Json::Value((UINT)m_hsocket);
	strHttpBody = Fast_Writer.write(JsonParam);

	//CONTENT-LENGTH:
	char szContent_length[64] ={0};
	LPCSTR pszContentLenght = "CONTENT-LENGTH: %d\r\n";
	sprintf_s(szContent_length, _countof(szContent_length), pszContentLenght,
		strHttpBody.length());
	strHttpHead += szContent_length;
	strHttpHead += "\r\n";

	// ����Ҫ���͵�����
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;

	// ����Push������
	DWORD	flags = 0;								//��־
	DWORD	dwSendBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_GETCHALLENGE;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// ��Ҫ�������ݸ��Ƶ�Buffer
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Send.szBuf;
	wsaBuf.len = strlen(m_IO_Send.szBuf);

	if (WSASend(m_hsocket,
		&wsaBuf,
		1,
		&dwSendBytes,
		flags,
		&m_IO_Send.overlapped,
		NULL) == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())			// �Ƿ�ɹ������ص�����
		{
			OutputDebugString(_T("getchallenge ����ʧ��!\n"));
			return FALSE;
		}
	}
	TCHAR szDebugstr[256] = {0};
	_stprintf_s(szDebugstr, _countof(szDebugstr),
		_T("Socket : %d  ����GetChallenge��TokenID�ǣ�%s\n"), (UINT)(m_hsocket), S2T(m_strTokenID).c_str());
	gvLog(LOG_WARNING, szDebugstr);
	return TRUE;

	/*

	))
	{
	}
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
	gvLog(LOG_WARNING, _T("getchallenge ����ʧ��!"));
	OutputDebugString(_T("getchallenge ����ʧ��"));
	return FALSE;
	}
	else
	{
	// ���ͳɹ�
	int nRecvBytes = SOCKET_ERROR;
	char szRecvBuf[512] = {0};
	while(SOCKET_ERROR == nRecvBytes )
	{
	nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
	if (SOCKET_ERROR == nRecvBytes)
	{
	// ����ʧ��
	int nErr = WSAGetLastError();
	TCHAR szBuf[128] = {0};
	TCHAR szTokenID[32] = {0};
	_stprintf_s(szBuf, _T("  Getchallenge ����recv����SOCKET_ERROR,TokenIDΪ: %s �������Ϊ��%d\n"), S2T(m_strTokenID),nErr);
	OutputDebugString(szBuf);
	return FALSE;
	}
	else if (nRecvBytes == 0)
	{
	TCHAR szBuf[128] = {0};
	TCHAR szTokenID[32] = {0};
	_stprintf_s(szBuf, _T("  Getchallenge ����recv����0, ���ӱ��رգ�TokenIDΪ: %s\n"), S2T(m_strTokenID));
	OutputDebugString(szBuf);
	return FALSE;
	}
	//if ( nRecvBytes == 0 ) 
	//{
	//	gvLog(LOG_WARNING, _T("getchallenge ���ش���!"));
	//	OutputDebugString(_T("getchallenge ���ش���\n"));
	//	return FALSE;
	//}
	else
	{
	// ���ճɹ�
	break;
	}	
	}
	// �����յ�������
	//	PSTR pRecvBuf = szRecvBuf;
	if (0==strlen(szRecvBuf))
	{
	gvLog(LOG_WARNING, _T("����getchallenge�󣬷�������������Ϊ��!"));
	OutputDebugString(_T("����getchallenge�󣬷�������������Ϊ��\n"));
	return FALSE;
	}
	string HttpBody = RemoveHttpHead(szRecvBuf);
	if(!HttpBody.empty())
	{
	//ȥ��Httpͷ�ɹ�
	Json::Value Root;
	Json::Reader Reader;
	if (Reader.parse(HttpBody.c_str(), Root))
	{
	string strRet =Root["ret"].asString();
	if (strRet == "0")
	{
	// ���سɹ�
	m_strChallenge = Root["chanllenge"].asString();
	if (!m_strChallenge.empty())
	{
	OutputDebugString(_T("chanllenge ���سɹ�!\n"));
	return TRUE;
	}
	else
	{
	gvLog(LOG_WARNING,_T("chanllenge����ֵΪ��!"));
	OutputDebugString(_T("chanllenge����ֵΪ��!\n"));
	return FALSE;
	}
	}
	else if (strRet == "1001")
	{
	// �������ܾ�����
	gvLog(LOG_WARNING,_T("getchallenge����ʧ�ܣ��������ܾ�����"));
	OutputDebugString(_T("getchallenge����ʧ�ܣ��������ܾ�����\n"));
	}
	else if(strRet == "1101")
	{
	// ���ݸ�ʽ����ȷ
	gvLog(LOG_WARNING,_T("getchallenge����ʧ�ܣ����ݸ�ʽ����ȷ"));
	OutputDebugString(_T("getchallenge����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
	}

	} 
	else
	{
	// Json��������challenge
	OutputDebugString(_T("Json��������challenge \n"));
	return FALSE;
	}
	}
	else
	{
	//�յ���Buffer��ʽ���󣬽���ʧ��
	gvLog(LOG_WARNING,_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��"));
	OutputDebugString(_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��\n"));
	return FALSE;
	}
	}	
	return TRUE;*/

}


// WsaSend Getchallenge���أ��ж��Ƿ�ɹ�����ɹ��������һ������
void CClientEndPoint::OnSendGetChallengeRet(DWORD dwIOBytesSize)
{
	// �ж�Getchallenge�����Ƿ�ɹ�
	if (dwIOBytesSize >0 )
	{
		// ���ͳɹ���ִ����һ������
		AsyncRecvGetChallenge();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send��������ʧ��
		gvLog(LOG_WARNING, _T("����GetChallenge ����Ϊ0�����ӿ��ܱ��ر�"));
		OutputDebugString(_T("����GetChallenge ����Ϊ0\n"));
	}
	else if (SOCKET_ERROR == dwIOBytesSize )
	{
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		gvLog(LOG_WARNING, strErrMsg.c_str());
		OutputDebugString(strErrMsg.c_str());
		
	}
}

// WsaRecv GetChallenge
BOOL  CClientEndPoint::AsyncRecvGetChallenge()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
	// ���շ��������ص�Challenge
	DWORD	flags = 0;								//��־
	DWORD	dwRecvBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_GETCHALLENGE;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;
	
	// ��ȡ����
	if (WSARecv(m_hsocket,
		&wsaBuf,
		1,
		&dwRecvBytes,
		&flags,
		&m_IO_Recv.overlapped,
		NULL)  == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			return FALSE;
		}
	} 
	return TRUE;		
}

// WsaRecv Getchallenge���أ��ж��Ƿ�ɹ�����ɹ��������һ������
void CClientEndPoint::OnRecvGetChallengeRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize>0)
	{

		TCHAR szDebugstr[256] = {0};
		_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  GetChallenge����\n"), (UINT)(m_hsocket) ) ;
		gvLog(LOG_WARNING, szDebugstr);

		// ���ճɹ�
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);
		// �����յ�������
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("����getchallenge�󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����getchallenge�󣬷�������������Ϊ��\n"));
			//return FALSE;
		}
		string HttpBody = RemoveHttpHead(szRecvBuf);
		if(!HttpBody.empty())
		{
			//ȥ��Httpͷ�ɹ�
			Json::Value Root;
			Json::Reader Reader;
			if (Reader.parse(HttpBody.c_str(), Root))
			{
				string strRet =Root["ret"].asString();
				if (strRet == "0")
				{
					// ���سɹ�
					g_challengesuccess +=1;
					m_strChallenge = Root["chanllenge"].asString();
					if (!m_strChallenge.empty())
					{
						OutputDebugString(_T("chanllenge ���سɹ�!\n"));
					//	int nRet = shutdown(m_hsocket, SD_SEND);
					//	StopSendMsg2PushSRV();

						// 2013-6-18   Added by Sanwen	
						if (GetChallenge == m_TestType)
						{
							// �ж�
							DisConnect2PushSRV();
						} 
						else
						{
							// ��������
							AsyncSendRegisterTokenID();
						}
						//						return TRUE;
					}
					else
					{
						gvLog(LOG_WARNING,_T("chanllenge����ֵΪ��!"));
						OutputDebugString(_T("chanllenge����ֵΪ��!\n"));
						//						return FALSE;
					}
				}
				else if (strRet == "1001")
				{
					// �������ܾ�����
					gvLog(LOG_WARNING,_T("getchallenge����ʧ�ܣ��������ܾ�����"));
					OutputDebugString(_T("getchallenge����ʧ�ܣ��������ܾ�����\n"));
				}
				else if(strRet == "1101")
				{
					// ���ݸ�ʽ����ȷ
					gvLog(LOG_WARNING,_T("getchallenge����ʧ�ܣ����ݸ�ʽ����ȷ"));
					OutputDebugString(_T("getchallenge����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
				}

			} 
			else
			{
				// Json��������challenge
				OutputDebugString(_T("Json��������challenge \n"));
				//				return FALSE;
			}
		}
		else
		{
			//�յ���Buffer��ʽ���󣬽���ʧ��
			gvLog(LOG_WARNING,_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��"));
			OutputDebugString(_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��\n"));
			//			return FALSE;
		}

	}
	else if ( 0 == dwIOBytesSize )
	{
		// Socket���ر�
		OutputDebugString(_T("RecvGetChallenge ����0\n"));

	}
	else if( SOCKET_ERROR == dwIOBytesSize)
	{
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}
}

// ע��TokenID
BOOL CClientEndPoint::AsyncSendRegisterTokenID()
{
	if (!m_bIsConnected)
	{
		
		return FALSE;
	}
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;

	// ����Http Request Head
	strHttpHead = HttpHead;

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// ����AppID ��CheckCode
	// MD5(SchoolID)-->AppID
	// MD5(TokenId + chanllenge + MD5(SchoolKey))-->CheckCode

	// AppID
	// char schoolId[64] = {0};
	// strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
	string Appid = Md5HexString(m_strSchoolID.c_str());

	// CheckCode
	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
	string strTemp = m_strTokenID;
	strTemp += m_strChallenge;
	strTemp += MD5SchoolKey;
	string CheckCode = Md5HexString(strTemp.c_str());

	// ����Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	BodyParam["appid"] = Appid;
	BodyParam["checkcode"] = CheckCode;
	BodyParam["model"] = "i9100";
	BodyParam["system"] = "Android2.2";
	BodyParam["appver"] = "1.2";
	BodyParam["SocketHandle"] = Json::Value((UINT)m_hsocket);
	Json::Value Root;
	Root["cmd"] = "register";
	Root["tokenid"] = m_strTokenID;
	Root["body"] = BodyParam;

	strHttpBody = Fast_Writer.write(Root);

	char szContent_length[64] ={0};
	LPCSTR pszContentLenght = "CONTENT-LENGTH: %d\r\n";
	sprintf_s(szContent_length, _countof(szContent_length), pszContentLenght,
		strHttpBody.length());
	strHttpHead += szContent_length;

	strHttpHead += "\r\n";

	// ����Ҫ���͵�����
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// ���͸�Push������
	DWORD	flags = 0;								//��־
	DWORD	dwSendBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_REGISTER_TOKENID;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// ��Ҫ�������ݸ��Ƶ�Buffer
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Send.szBuf;
	wsaBuf.len = strlen(m_IO_Send.szBuf);
	//wsaBuf.len = DATA_BUF_SIZE;

	if (WSASend(m_hsocket,
		&wsaBuf,
		1,
		&dwSendBytes,
		flags,
		&m_IO_Send.overlapped,
		NULL) == SOCKET_ERROR)
	{
		DWORD dwErr = GetLastError();
		if (ERROR_IO_PENDING != WSAGetLastError())			// �ɹ������ص�����
		{
			OutputDebugString(_T("RegisterTokenID ����ʧ��!\n"));
			return FALSE;
		}
	}

	TCHAR szDebugstr[256] = {0};
	_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  ����RegisterTokenID\n"), (UINT)(m_hsocket) ) ;
	gvLog(LOG_WARNING, szDebugstr);
	return TRUE;

	/*


	// ����Push������
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
	gvLog(LOG_WARNING, _T("RegisterTokenID ����ʧ��"));
	OutputDebugString(_T("RegisterTokenID ����ʧ��\n"));
	return FALSE;
	}
	else
	{
	// ���ͳɹ�
	int nRecvBytes = SOCKET_ERROR;
	char szRecvBuf[512] = {0};
	while(SOCKET_ERROR == nRecvBytes )
	{
	nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
	if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
	{
	gvLog(LOG_WARNING, _T("RegisterTokenID recv���ش���"));
	OutputDebugString(_T("RegisterTokenID recv���ش���\n"));
	return FALSE;
	}
	else
	{
	// ���ճɹ�
	break;
	}	
	}
	// �����յ�������
	//	PSTR pRecvBuf = szRecvBuf;
	if (0==strlen(szRecvBuf))
	{
	gvLog(LOG_WARNING, _T("����RegisterTokenID �󣬷�������������Ϊ��!"));
	OutputDebugString(_T("����RegisterTokenID �󣬷�������������Ϊ��\n"));
	return FALSE;
	}

	string HttpBody = RemoveHttpHead(szRecvBuf);
	if(!HttpBody.empty())
	{
	//ȥ��Httpͷ�ɹ�
	Json::Value RecvRoot;
	Json::Reader Reader;
	if (Reader.parse(HttpBody, RecvRoot))
	{
	string strRet = RecvRoot["ret"].asString();
	if (strRet=="0")
	{
	OutputDebugString(_T("RegisterTokenID�����������سɹ�\n"));
	return TRUE;

	}
	else if (strRet == "1001")
	{
	// �������ܾ�����
	gvLog(LOG_WARNING,_T("RegisterTokenID ����ʧ�ܣ��������ܾ�����"));
	OutputDebugString(_T("RegisterTokenID ����ʧ�ܣ��������ܾ�����\n"));
	return FALSE;
	}
	else if(strRet == "1101")
	{

	// ���ݸ�ʽ����ȷ
	gvLog(LOG_WARNING,_T("RegisterTokenID ����ʧ�ܣ����ݸ�ʽ����ȷ"));
	OutputDebugString(_T("RegisterTokenID ����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
	return FALSE;
	}
	} 
	else
	{
	// Json��������challenge
	OutputDebugString(_T("Json��������challenge \n"));
	return FALSE;
	}
	}
	else
	{
	//�յ���Buffer��ʽ���󣬽���ʧ��
	OutputDebugString(_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��\n"));
	return FALSE;
	}
	}	
	return TRUE;*/

}

void CClientEndPoint::OnSendRegisterTokenIDRet(DWORD dwIOBytesSize)
{
	// �ж�Getchallenge�����Ƿ�ɹ�
	if (dwIOBytesSize >0 )
	{
		// ���ͳɹ���ִ����һ������
		AsyncRecvRegisterTokenID();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send��������ʧ��
		OutputDebugString(_T("����GetChallenge ����ʧ��\n"));
	}
	else if (SOCKET_ERROR == dwIOBytesSize )
	{
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}

}

BOOL CClientEndPoint::AsyncRecvRegisterTokenID()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
	// ���շ��������ص�Challenge
	DWORD	flags = 0;								//��־
	DWORD	dwRecvBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_REGESTER_TOKENID;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;

	// ��ȡ����
	if (WSARecv(m_hsocket,
		&wsaBuf,
		1,
		&dwRecvBytes,
		&flags,
		&m_IO_Recv.overlapped,
		NULL)  == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			return FALSE;
		}
	} 
	return TRUE;		
}

void CClientEndPoint::OnRecvRegisterTokenIDRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize>0)
	{
		TCHAR szDebugstr[256] = {0};
		_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  RegisterTokenID����\n"), (UINT)(m_hsocket) ) ;
		gvLog(LOG_WARNING, szDebugstr);
		// ���ճɹ�
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);

		// �����յ�������
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("����RegisterTokenID �󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����RegisterTokenID �󣬷�������������Ϊ��\n"));
			//	return FALSE;
		}

		string HttpBody = RemoveHttpHead(szRecvBuf);
		if(!HttpBody.empty())
		{
			//ȥ��Httpͷ�ɹ�
			Json::Value RecvRoot;
			Json::Reader Reader;
			if (Reader.parse(HttpBody, RecvRoot))
			{
				string strRet = RecvRoot["ret"].asString();
				if (strRet=="0")
				{
					g_registersuccess +=1;
					OutputDebugString(_T("RegisterTokenID�����������سɹ�\n"));

					// 2013-6-18   Added by Sanwen	
					if (RegisterToken == m_TestType)
					{
						// �ж�
						DisConnect2PushSRV();
					} 
					else
					{
						// ��������
						AsyncSendPushMessage();
					}
					//	return TRUE;

				}
				else if (strRet == "1001")
				{
					// �������ܾ�����
					gvLog(LOG_WARNING,_T("RegisterTokenID ����ʧ�ܣ��������ܾ�����"));
					OutputDebugString(_T("RegisterTokenID ����ʧ�ܣ��������ܾ�����\n"));
					//	return FALSE;
				}
				else if(strRet == "1101")
				{

					// ���ݸ�ʽ����ȷ
					gvLog(LOG_WARNING,_T("RegisterTokenID ����ʧ�ܣ����ݸ�ʽ����ȷ"));
					OutputDebugString(_T("RegisterTokenID ����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
					SOCKADDR_IN sock_addr;
					int nLen = sizeof(SOCKADDR_IN);
					getsockname(m_hsocket,(SOCKADDR*)&sock_addr, &nLen);
					OutputDebugString(_T("�����˿��ǣ�"));
					OutputDebugString(itot(ntohs(sock_addr.sin_port), 10).c_str());
					OutputDebugString(S2T(HttpBody).c_str());
					//		return FALSE;
				}
			} 
			else
			{
				// Json��������challenge
				OutputDebugString(_T("Json��������RegisterTokenID���ص�ֵ \n"));
				//	return FALSE;
			}
		}
		else
		{
			//�յ���Buffer��ʽ���󣬽���ʧ��
			OutputDebugString(_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��\n"));
			//	return FALSE;
		}
	}
	else if (dwIOBytesSize == 0)
	{
		// Socket���ر�
	}
	else
	{
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}

}


// ����Push��Ϣ
BOOL CClientEndPoint::AsyncSendPushMessage()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;

	// ����Http Request Head
	strHttpHead = HttpHead;

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// ����Http Body
	Json::Value JsonParam;
	Json::FastWriter Fast_Writer;
	JsonParam["cmd"] = Json::Value("getpushmessage");
	strHttpBody = Fast_Writer.write(JsonParam);

	//CONTENT-LENGTH:
	char szContent_length[64] ={0};
	LPCSTR pszContentLenght = "CONTENT-LENGTH: %d\r\n";
	sprintf_s(szContent_length, _countof(szContent_length), pszContentLenght,
		strHttpBody.length());
	strHttpHead += szContent_length;
	strHttpHead += "\r\n";

	// ����Ҫ���͵�����
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// ���͸�Push������
	DWORD	flags = 0;								//��־
	DWORD	dwSendBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_GETPUSHMESSAGE;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// ��Ҫ�������ݸ��Ƶ�Buffer
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Send.szBuf;
	wsaBuf.len = strlen(m_IO_Send.szBuf);

	if (WSASend(m_hsocket,
		&wsaBuf,
		1,
		&dwSendBytes,
		flags,
		&m_IO_Send.overlapped,
		NULL) == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())			// �ɹ������ص�����
		{
			OutputDebugString(_T("RegisterTokenID ����ʧ��!\n"));
			return FALSE;
		}
	}
	//StopSendMsg2PushSRV();
	return TRUE;


	/*
	// ����Push������
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
	gvLog(LOG_WARNING, _T("getpushmessage ����ʧ��!"));
	OutputDebugString(_T("getpushmessage ����ʧ��"));
	return FALSE;
	}
	else
	{
	// ���ͳɹ�
	int nRecvBytes = SOCKET_ERROR;
	char szRecvBuf[512] = {0};
	while(SOCKET_ERROR == nRecvBytes)
	{
	nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
	if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
	{
	gvLog(LOG_WARNING, _T("getpushmessage ���ش���!"));
	OutputDebugString(_T("getpushmessage ���ش���\n"));
	return FALSE;
	}
	else
	{
	// ���ճɹ�
	break;
	}	
	}
	// �����յ�������
	//	PSTR pRecvBuf = szRecvBuf;
	if (0==strlen(szRecvBuf))
	{
	gvLog(LOG_WARNING, _T("����getpushmessage�󣬷�������������Ϊ��!"));
	OutputDebugString(_T("����getpushmessage�󣬷�������������Ϊ��\n"));
	return FALSE;
	}
	string strRecvBuf(szRecvBuf);
	string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");

	if (nIdex != string::npos)//���سɹ�
	{
	OutputDebugString(_T("����getpushmessage�󣬷��������سɹ�\n"));
	return TRUE;
	}
	else
	{
	return FALSE;
	}
	}	*/

	//return TRUE;
}

void CClientEndPoint::OnSendPushMessageRet(DWORD dwIOBytesSize)
{
	// �ж�SendPushMessage�����Ƿ�ɹ�
	if (dwIOBytesSize >0 )
	{
		// ���ͳɹ���ִ����һ������
//	int nRet = shutdown(m_hsocket, SD_SEND);
	//	int nRet = shutdown(m_hsocket,SD_SEND);

		// SendPushMessage��״̬��Login��Ϊonline
		m_CurrentStatus = online;
		m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOfflineIntervalTime,
			m_TimeShreshold.nMaxOfflineIntervalTime);
		m_nCurrentTime = 0;
		AsyncRecvPushMessage();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send��������ʧ��
		OutputDebugString(_T("����GetChallenge ����ʧ��\n"));
	}
	else if (SOCKET_ERROR == dwIOBytesSize )
	{
		// 
	}
}

BOOL CClientEndPoint::AsyncRecvPushMessage()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
	// ����Recv PushMessage��������
	DWORD	flags = 0;								//��־
	DWORD	dwRecvBytes =0;					//�����ֽ���
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_PUSHMESSAGE;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;

	// ��ȡ����
	if (WSARecv(m_hsocket,
		&wsaBuf,
		1,
		&dwRecvBytes,
		&flags,
		&m_IO_Recv.overlapped,
		NULL)  == SOCKET_ERROR)
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			return FALSE;
		}
	} 
	return TRUE;		
}
void CClientEndPoint::OnRecvPushMessageRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize>0)
	{
		// ���ճɹ�
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);

		// �����յ�������
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("����getpushmessage�󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����getpushmessage�󣬷�������������Ϊ��\n"));
			//return FALSE;
		}
		string strRecvBuf(szRecvBuf);
		string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
	
		if (nIdex != string::npos)//���سɹ�
		{
			OutputDebugString(_T("����getpushmessage�󣬷��������سɹ�\n"));

			// 2013-6-18   Added by Sanwen	
			if (GetPushMsg == m_TestType)
			{
				// �ж�
				DisConnect2PushSRV();
			} 
			else
			{
				// ��������
				// ����Recv PushMessage��������
				if (m_bIsConnected)
				{
					DWORD	flags = 0;								//��־
					DWORD	dwRecvBytes =0;					//�����ֽ���
					ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
					m_IO_Recv.optype = RECV_PUSHMESSAGE;
					WSABUF wsaBuf;
					wsaBuf.buf = m_IO_Recv.szBuf;
					wsaBuf.len = DATA_BUF_SIZE;
					WSARecv(m_hsocket,
						&wsaBuf,
						1,
						&dwRecvBytes,
						&flags,
						&m_IO_Recv.overlapped,
						NULL) ;
				}

			}
						


		//	RecvPushMessage();
			//	return TRUE;
		}
		else
		{
			//return FALSE;
		}

	}
	else if (dwIOBytesSize == 0)
	{
		// Socket���ر�
	}
	else
	{
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}

}
// �����շ���������������Ϣ
void CClientEndPoint::RecvPushMessage(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize >0 )
	{
		g_pushmessagesuccess +=1;
		// �ɹ�
		// ������
		string strRecvBuf(pIO_Data->szBuf, dwIOBytesSize);
		string strRecvBody;
		string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
		if (nIdex != string::npos)//���سɹ�
		{
			// ȥ��httpͷ
			strRecvBody.clear();
			strRecvBody = RemoveHttpHead(strRecvBuf.c_str());
			OutputDebugString(_T("����getpushmessage�󣬷��������سɹ�\n"));
		}
		else
		{
			// �Ѿ���Body������ȥHttpͷ
			strRecvBody = strRecvBuf;
		}

		if (strRecvBody.length() > 0)
		{
			// ��������
			PushMsgList MsgList;
			ParsePushMessge(strRecvBody.c_str(), MsgList);

			// �������ݿ�
			for (PushMsgList::iterator iter = MsgList.begin();
				iter != MsgList.end(); ++ iter)
			{
				if (	ClientDB::GetInstance()->InsertPushMsg2DB(iter->strTokenID,iter->strBody))
				{
					InterlockedIncrement(&g_nPushMsgNum);
				}						
			}
		}
	//	if (online == m_CurrentStatus)
	//	{
			// ����Recv PushMessage��������
			DWORD	flags = 0;								//��־
			DWORD	dwRecvBytes =0;					//�����ֽ���
			ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
			m_IO_Recv.optype = RECV_PUSHMESSAGE;
			WSABUF wsaBuf;
			wsaBuf.buf = m_IO_Recv.szBuf;
			wsaBuf.len = DATA_BUF_SIZE;
			WSARecv(m_hsocket,
				&wsaBuf,
				1,
				&dwRecvBytes,
				&flags,
				&m_IO_Recv.overlapped,
				NULL) ;

//		}


	}
	else if (0== dwIOBytesSize)
	{
		// ʧ�ܣ����ӿ��ܱ��ر�

	} 
	else
	{
		// SOCKET ERROR
		// ����
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());

	}

}

// ����nOpType��ѡ����һ���Ĳ���
DWORD CClientEndPoint::SeleteOperation(LPOVERLAPPED lpOverlapped, DWORD dwIOBytesSize)
{

	//��ȡ��չ�ص��ṹָ��
	PIO_OPERATION_DATA pIO = CONTAINING_RECORD(lpOverlapped,	
		IO_OPERATION_DATA,
		overlapped);

	switch(pIO->optype)
	{
		// GetChallenge
	case SEND_GETCHALLENGE:
		OnSendGetChallengeRet(dwIOBytesSize);
		break;
	case RECV_GETCHALLENGE:
		OnRecvGetChallengeRet(pIO, dwIOBytesSize);
		break;

		// RegisterTokenId
	case SEND_REGISTER_TOKENID:
		OnSendRegisterTokenIDRet(dwIOBytesSize);
		break;
	case RECV_REGESTER_TOKENID:
		OnRecvRegisterTokenIDRet(pIO, dwIOBytesSize);
		break;

		// GetPushMessage
	case SEND_GETPUSHMESSAGE:
		OnSendPushMessageRet(dwIOBytesSize);
		break;
	case RECV_GETPUSHMESSAGE:
		OnRecvPushMessageRet(pIO, dwIOBytesSize);
		break;

		// ѭ������PushMessage	
	case RECV_PUSHMESSAGE:
		RecvPushMessage(pIO, dwIOBytesSize);
		break;
	default:
		break;
	}
	return 1;
}

// ֹͣ��PushServer������Ϣ
BOOL CClientEndPoint::StopSendMsg2PushSRV()			
{
	//CancelIo((HANDLE)m_hsocket);
	//closesocket(m_hsocket);
	//m_CurrentStatus == logoff;
	//m_hsocket = INVALID_SOCKET;
	//return TRUE;
	if (0 == shutdown(m_hsocket, SD_SEND))
	{
		m_CurrentStatus == logoff;
		return TRUE;
	} 
	else
	{
		DWORD dwErr = GetLastError();
		TSTRING strErrInfo = GetErrCodeInfo(dwErr);
		OutputDebugString(strErrInfo.c_str());
		return FALSE;
	}

	//if (0== shutdown(m_hsocket, SD_SEND))
	//{
	//	// ���շ��������ص�Challenge
	//	DWORD	flags = 0;								//��־
	//	DWORD	dwRecvBytes =0;					//�����ֽ���
	//	WSABUF wsaBuf;
	//	wsaBuf.len = 0;
	//	wsaBuf.buf = NULL;


	//	// ��ȡ����
	//	if (WSARecv(m_hsocket,
	//		&wsaBuf,
	//		1,
	//		&dwRecvBytes,
	//		&flags,
	//		&m_IO_Recv.overlapped,
	//		NULL)  == SOCKET_ERROR)
	//	{
	//		DWORD dwErr = GetLastError();
	//		if (ERROR_IO_PENDING != dwErr)
	//		{
	//			return FALSE;
	//		}
	//	} 

	//	return TRUE;
	//} 
	//else
	//{
	//	DWORD dwErr = GetLastError();
	//	TSTRING strErrInfo = GetErrCodeInfo(dwErr);
	//	OutputDebugString(strErrInfo.c_str());
	//	return FALSE;
	//}
}

// �Ͽ�Push������������
BOOL CClientEndPoint::DisConnect2PushSRV()
{
	if (INVALID_SOCKET != m_hsocket)
	{
		closesocket(m_hsocket);
		m_hsocket = INVALID_SOCKET;
	}
	m_bIsConnected = FALSE;
	m_CurrentStatus = offline;
	m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOnlineIntervalTime,
																				m_TimeShreshold.nMaxOnlineIntervalTime);
	m_nCurrentTime = 0;
	return TRUE;
}

/*
// ��ʼ�������ݽ��ղ���
int  CClientEndPoint::StartRecvDataTest()
{
m_hTreadRecvData = ::CreateThread(NULL, 0, RecvDataThread, this, 0,0 );
//CloseHandle(m_hTreadRecvData);
return 0;
}*/



//// ֹͣ���ݽ��ղ���
//int CClientEndPoint::StopRecvDataTest()
//{
//	m_bIsThreadshouldAbort = TRUE;
//	DisConnect2PushSRV();
//	WaitForSingleObject(m_hTreadRecvData, INFINITE);
//	CloseHandle(m_hTreadRecvData);
//	return 0;
//}

// ���ó�ʱ
BOOL CClientEndPoint::SetTimeOut(UINT nTimeOut)
{
	if (INVALID_SOCKET != m_hsocket && nTimeOut >0 )
	{
		// ���÷��ͳ�ʱ
		if (SOCKET_ERROR == ::setsockopt(m_hsocket,
			SOL_SOCKET,
			SO_SNDTIMEO,
			(char*)(&nTimeOut),
			sizeof(nTimeOut) )  )
		{
			return FALSE;
		}
		// ���ý��ճ�ʱ
		if (SOCKET_ERROR == ::setsockopt(m_hsocket,
			SOL_SOCKET,
			SO_RCVTIMEO,
			(char*)(&nTimeOut),
			sizeof(nTimeOut) )  )
		{
			return FALSE;
		}
		return TRUE;		
	} 
	else
	{
		gvLog(LOG_ERR_USER, _T("���ó�ʱʧ�ܣ�Socket��Ч��ʱ��������!"));
		OutputDebugString(_T("���ó�ʱʧ�ܣ�Socket��Ч��ʱ��������!!\n"));
		return FALSE;
	}
}
string CClientEndPoint::RemoveHttpHead(LPCSTR pszRecvBuf)
{
	string temp(pszRecvBuf);
	string::size_type nIndex =  temp.find("\r\n\r\n");
	if (string::npos != nIndex)
	{
		// �ҵ�
		nIndex +=4;		//�Ƶ����
		string HttpBody = temp.substr(nIndex, (temp.length() - nIndex));	
		return HttpBody;
	}
	else
		return temp;
}

// ��������
BOOL CClientEndPoint::ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList)
{
	BOOL bResult = FALSE;

	// ���յ������ݼ������
	m_strRecvBuffer += pszRecvBuf;

	CStringA strRecvBuf(m_strRecvBuffer.c_str());

	// ѭ������
	while(-1 != strRecvBuf.Find("Context-Length:") )
	{
		int nBeginPos = -1;
		int nEndPos = -1;
		nBeginPos = strRecvBuf.Find("Context-Length:");
		nBeginPos += 15;			//����Content-Length;֮��
		nEndPos = strRecvBuf.Find("\r\n\r\n", nBeginPos);
		if (nEndPos != -1)
		{
			CStringA strJsonLength = strRecvBuf.Mid(nBeginPos, (nEndPos-nBeginPos));
			strJsonLength.Trim();
			int nJsonLength = atoi(strJsonLength);			//�����Json���ݶεĳ���
			nBeginPos = nEndPos + 4;
			nEndPos = nBeginPos + nJsonLength;
			CStringA strJson = strRecvBuf.Mid(nBeginPos, nJsonLength);

			// strJson�Ƿ��������
			if (strJson.GetLength() == nJsonLength)
			{
				// ���ճɹ�, ���ö�Json��Buffer��ɾȥ
				strRecvBuf.Delete(0, nEndPos);	

				// Json�����������Խ���
				LPCSTR pszJson = strJson.GetBuffer();
				Json::Reader reader;
				Json::FastWriter Fast_Writer;
				Json::Value root;
				if (reader.parse(pszJson, root))
				{
					// �����ɹ�, 
					// ����PushMsgList
					PUSHMESSAGE PushMsg;
					PushMsg.strTokenID = root["tokenid"].asString();
					PushMsg.strBody = Fast_Writer.write(root["body"]);
					PushMessageList.push_back(PushMsg);
					bResult = TRUE;
					strJson.ReleaseBuffer();
				}
				else
				{
					// �������ɹ�����ʽ���ܴ���
					strJson.ReleaseBuffer();
					// д��Log, PushMessage Json����ʧ��
					char szLog[512] = {0};
					sprintf_s(szLog, "�յ���PushMessage ��Json����ʧ�� ��%s", pszJson);
					gvLog(LOG_WARNING, S2T(szLog).c_str());
					OutputDebugString(S2T(szLog).c_str());	
				}

			} 
			else
			{
				// Json�β��������˳�Whileѭ�����������ݶ��У�
				// �´����յ�����ʱ�ٽ��н���
				break;
			}
		}
		else
		{
			break;
		}
	}
	m_strRecvBuffer.clear();
	m_strRecvBuffer = strRecvBuf.GetBuffer();
	strRecvBuf.ReleaseBuffer();
	return bResult;
}

