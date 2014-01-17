#include "StdAfx.h"
#include "ServerEndPoint.h"
#include "LogFile.h"

#define DEFAULT_IP _T("127.0.0.1")

CServerEndPoint::CServerEndPoint(void):
m_bIsConnected(FALSE),
m_bIsTimeOut(FALSE),
m_socket(INVALID_SOCKET),
m_pListener(NULL)
{
}

CServerEndPoint::~CServerEndPoint(void)
{
}
////����SchoolID��SchoolKey
//void CServerEndPoint:: SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey)
//{
//	ASSERT(!strSchoolID.empty());
//	ASSERT(!strSchoolKey.empty());
//	this->m_strSchoolID = strSchoolID;
//	this->m_strSchoolKey = strSchoolKey;	
//}


// ���ûص�
BOOL CServerEndPoint::AddListener(IServerEndPointNotify *ptr)
{
	if (ptr == NULL || m_pListener != NULL)
	{
		return FALSE;
	} 
	else
	{
		m_pListener = ptr;
		return TRUE;
	}
}
// ȡ���ص�
BOOL CServerEndPoint::RemoveListener(IServerEndPointNotify *ptr)
{
	if (ptr == m_pListener)
	{
		m_pListener = NULL;
		return TRUE;
	} 
	else
	{
		return FALSE;
	}
}



// ����AppID��AppKey
void CServerEndPoint::SetAppInfo(const string& strAppID, const string& strAppKey)
{
	ASSERT(!strAppID.empty());
	ASSERT(!strAppKey.empty());
	this->m_strAppID = strAppID;
	this->m_strAppKey = strAppKey;	
}

// ����TokenID
void CServerEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	this->m_strTokenID = strTokenID;
}

//���ӵ�Push������
BOOL CServerEndPoint::Connect2PushSRV(DWORD dwServerIP, UINT nPort)
{
	if (INVALID_SOCKET == m_socket)
	{
		m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} 
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);

	// ��λ���ӱ�־
	m_bIsConnected = FALSE;
	// ��λ��ʱ��־
	m_bIsTimeOut = FALSE;

	if (0 != ::connect(m_socket, (SOCKADDR*)&(m_PushServerAddr),sizeof(m_PushServerAddr) ))
	{
		// ����ʧ��
		int nErr = WSAGetLastError();
		return FALSE;
	}
	else
	{
		SOCKADDR_IN LocalAddr;
		int namelen = sizeof(LocalAddr);
		getsockname(m_socket, (SOCKADDR*)&LocalAddr, &namelen);
		TCHAR szBuf[64] = {0};
		_stprintf(szBuf, _T("�����Ķ˿�Ϊ��%d \n"), ntohs(LocalAddr.sin_port));
		OutputDebugString(szBuf);
		// ���ӳɹ������������߳�
		m_bIsConnected = TRUE;

		return TRUE;
	}	

}
BOOL CServerEndPoint::SetTimeOut(UINT nTimeOut)
{
	if (INVALID_SOCKET != m_socket && nTimeOut >0 )
	{
		// ���÷��ͳ�ʱ
		if (SOCKET_ERROR == ::setsockopt(m_socket,
																SOL_SOCKET,
																SO_SNDTIMEO,
																(char*)(&nTimeOut),
																sizeof(nTimeOut) )  )
		{
			return FALSE;
		}
		// ���ý��ճ�ʱ
		if (SOCKET_ERROR == ::setsockopt(m_socket,
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

// �Ͽ�Push������������
BOOL CServerEndPoint::DisConnect2PushSRV()
{
	int nRet = ::shutdown(m_socket,SD_SEND);
	if (0 == nRet)
	{
		m_bIsConnected = FALSE;
		::closesocket(m_socket);
		m_socket =INVALID_SOCKET;
		return TRUE;
	}
	else
	{
		gvLog(LOG_WARNING, _T("�Ͽ�Push������ʧ��!"));
		OutputDebugString(_T("�Ͽ�Push������ʧ�ܣ�\n"));
		return FALSE;
	}
}

//����Challenge
BOOL CServerEndPoint::GetChallenge()
{
	ASSERT(m_bIsConnected);
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// ����Http Request Head

	strHttpHead += "POST / HTTP/1.1\r\n";
	strHttpHead += "CHARSET: UTF-8\r\n";
	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
		strHttpHead += szHostInfoBuf;

	// ����Http Body
	Json::Value JsonParam;
	Json::FastWriter Fast_Writer;
	JsonParam["cmd"] = Json::Value("getserverchanllenge");
	SYSTEMTIME tm;
	GetLocalTime(&tm);
	char szTime[64] = {0};
	sprintf_s(szTime,"%d-%d-%d-%d",tm.wHour,tm.wMinute,tm.wSecond,tm.wSecond);
	JsonParam["Time"] = Json::Value(szTime);
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
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("getchallenge ����ʧ��!"));
		OutputDebugString(_T("getchallenge ����ʧ��"));
		OutputDebugString(S2T(szTime).c_str());
		return FALSE;
	}
	else
	{
		// ���ͳɹ�
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("getchallenge ���ش���!"));
				OutputDebugString(_T("getchallenge ���ش���\n"));
				OutputDebugString(S2T(szTime).c_str());
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
			gvLog(LOG_WARNING, _T("����getchallenge�󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����getchallenge�󣬷�������������Ϊ��\n"));
			OutputDebugString(S2T(szTime).c_str());
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
	return FALSE;
}
//ע��Server(��½Push������)
BOOL  CServerEndPoint::LoginServer()
{
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// ����Http Request Head

	strHttpHead += "POST / HTTP/1.1\r\n";
	strHttpHead += "CHARSET: UTF-8\r\n";
	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";

	// ����AppID ��CheckCode
	// MD5(SchoolID)-->AppID
	// MD5(chanllenge+MD5(SchoolKey))-->CheckCode
	
	// AppID
	//char schoolId[64] = {0};
	//strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
//	string Appid = Md5HexString(m_strSchoolID.c_str());
	
	//// CheckCode

//	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
	string strTemp = m_strChallenge;
	strTemp += m_strAppKey;
	string CheckCode = Md5HexString(strTemp.c_str());

	// ����Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	BodyParam["appid"] = m_strAppID;
	BodyParam["checkcode"] = CheckCode;
	Json::Value Root;
	Root["cmd"] = "register";
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


	// ����Push������
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("RegisterServer ����ʧ��"));
		OutputDebugString(_T("RegisterServer ����ʧ��\n"));
		return FALSE;
	}
	else
	{
		// ���ͳɹ�
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("RegisterServer recv���ش���"));
				OutputDebugString(_T("RegisterServer recv���ش���\n"));
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
			gvLog(LOG_WARNING, _T("����RegisterServer�󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����RegisterServer�󣬷�������������Ϊ��\n"));
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
					OutputDebugString(_T("RegisterServer�����������سɹ�\n"));
					return TRUE;
			
				}
				else if (strRet == "1001")
				{
					// �������ܾ�����
					gvLog(LOG_WARNING,_T("RegisterServer����ʧ�ܣ��������ܾ�����"));
					OutputDebugString(_T("RegisterServer����ʧ�ܣ��������ܾ�����\n"));
					return FALSE;
				}
				else if(strRet == "1101")
				{

					// ���ݸ�ʽ����ȷ
					gvLog(LOG_WARNING,_T("RegisterServer����ʧ�ܣ����ݸ�ʽ����ȷ"));
					OutputDebugString(_T("RegisterServer����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
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
	
	return TRUE;
}
// ���ʹ����CheckCode
//BOOL CServerEndPoint::SendErrorCheckCode2PushSRV()
//{
//
//	string strHttpPost;
//	string strHttpHead;
//	string strHttpBody;
//	// ����Http Request Head
//
//	strHttpHead += "POST / HTTP/1.1\r\n";
//	strHttpHead += "CHARSET: UTF-8\r\n";
//	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
//	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
//	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";
//
//	// ����AppID ��CheckCode
//	// MD5(SchoolID)-->AppID
//	// MD5(chanllenge+MD5(SchoolKey))-->CheckCode
//
//	// AppID
//	//char schoolId[64] = {0};
//	//strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
//	string Appid = Md5HexString(m_strSchoolID.c_str());
//
//	//// CheckCode
//
//	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
//	//string strTemp = m_strChallenge;
//	//strTemp += MD5SchoolKey;
//	string strTemp = MD5SchoolKey;
//	string CheckCode = Md5HexString(strTemp.c_str());
//
//	// ����Http Body---Json
//	Json::Value BodyParam;
//	Json::FastWriter Fast_Writer;
//	BodyParam["appid"] = Appid;
//	BodyParam["checkcode"] = CheckCode;
//	Json::Value Root;
//	Root["cmd"] = "register";
//	Root["body"] = BodyParam;
//
//	strHttpBody = Fast_Writer.write(Root);
//
//	char szContent_length[64] ={0};
//	LPCSTR pszContentLenght = "CONTENT-LENGTH: %d\r\n";
//	sprintf_s(szContent_length, _countof(szContent_length), pszContentLenght,
//		strHttpBody.length());
//	strHttpHead += szContent_length;
//
//	strHttpHead += "\r\n";
//
//	// ����Ҫ���͵�����
//	strHttpPost += strHttpHead;
//	strHttpPost += strHttpBody;
//
//
//	// ����Push������
//	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);
//
//	if(SOCKET_ERROR == nSendBytes)
//	{
//		gvLog(LOG_WARNING, _T("ErrorCheckCode ����ʧ��"));
//		OutputDebugString(_T("ErrorCheckCode ����ʧ��\n"));
//		return FALSE;
//	}
//	else
//	{
//		// ���ͳɹ�
//		int nRecvBytes = SOCKET_ERROR;
//		char szRecvBuf[512] = {0};
//		while(SOCKET_ERROR == nRecvBytes )
//		{
//			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
//			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
//			{
//				gvLog(LOG_WARNING, _T("ErrorCheckCode ���մ���"));
//				OutputDebugString(_T("ErrorCheckCode ���մ���\n"));
//				return FALSE;
//			}
//			else
//			{
//				// ���ճɹ�
//				break;
//			}	
//		}
//		// �����յ�������
//		//	PSTR pRecvBuf = szRecvBuf;
//		if (0==strlen(szRecvBuf))
//		{
//			gvLog(LOG_WARNING, _T("����ErrorCheckCode�󣬷�������������Ϊ��!"));
//			OutputDebugString(_T("����ErrorCheckCode�󣬷�������������Ϊ��\n"));
//			return FALSE;
//		}
//		string HttpBody = RemoveHttpHead(szRecvBuf);
//		if(!HttpBody.empty())
//		{
//			//ȥ��Httpͷ�ɹ�
//			Json::Value RecvRoot;
//			Json::Reader Reader;
//			if (Reader.parse(HttpBody, RecvRoot))
//			{
//				string strRet = RecvRoot["ret"].asString();
//				if (!strRet.empty())
//				{
//					if (strRet=="2001")
//					{
//						OutputDebugString(_T("����ErrorCheckCode�󣬷���������CheckCode������ȷ��\n"));
//						return TRUE;
//					}
//					else if (strRet == "0")
//					{
//						gvLog(LOG_WARNING,_T("����ErrorCheckCode�󣬷��������سɹ�"));
//						OutputDebugString(_T("����ErrorCheckCode�󣬷��������سɹ��������⣩\n"));
//						return TRUE;
//
//					}
//					else if (strRet == "1101")
//					{
//
//						// ���ݸ�ʽ����ȷ
//						gvLog(LOG_WARNING,_T("ErrorCheckCode����ʧ�ܣ����ݸ�ʽ����ȷ"));
//						OutputDebugString(_T("ErrorCheckCode����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
//						return FALSE;
//
//					}
//					else if (strRet == "1001")
//					{
//						// �������ܾ�����
//						gvLog(LOG_WARNING,_T("ErrorCheckCode����ʧ�ܣ��������ܾ�����"));
//						OutputDebugString(_T("ErrorCheckCode����ʧ�ܣ��������ܾ�����\n"));
//						return FALSE;
//
//					}
//				}
//
//			} 
//			else
//			{
//				// Json��������RegisterServer ���ص�����
//				OutputDebugString(_T("Json��������RegisterServer ���ص����� \n"));
//				return FALSE;
//			}
//		}
//		else
//		{
//			//�յ���Buffer��ʽ���󣬽���ʧ��
//			OutputDebugString(_T("�յ���Buffer��ʽ����ȥ��Httpͷʧ��\n"));
//			return FALSE;
//		}
//	}	
//
//	return TRUE;
//}

BOOL CServerEndPoint::SendPushMessage(/*string strTokenID, */string strBody)
{

	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// ����Http Request Head

	strHttpHead += "POST / HTTP/1.1\r\n";
	strHttpHead += "CHARSET: UTF-8\r\n";
	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";

	// ����Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	Json::Reader Reader;
	Json::Value TokenIdArr;
	Json::Value TokenId;
	Json::Value TokenBody;
	
	if (!Reader.parse(strBody, TokenBody))
	{
		OutputDebugString(_T("Body��ʽ����Json��������"));
		if (NULL !=m_pListener)
		{
			TSTRING strErr = _T("Ҫ���͵�����Body��ʽ����Json�������ˣ�����ʧ�ܣ�");
			m_pListener->OnErrorSend(T2S(strErr));
		}
		return FALSE;
	}
	
	TokenId["tokenid"] = Json::Value(m_strTokenID);
	TokenIdArr.append(TokenId);

	BodyParam["tokenidlist"] =TokenIdArr;
	BodyParam["body"] = TokenBody;
	Json::Value Root;
	Root["cmd"] = "pushmsg";
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

	// ����Push������
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		if (NULL != m_pListener)
		{
			m_pListener->OnErrorSend(strHttpPost);
		}
		OutputDebugString(_T("PushMessage ����ʧ��"));
		return FALSE;
	}
	else
	{
		if (NULL != m_pListener)
		{
			m_pListener->OnSuccessSend(strHttpPost);
		}
		// ���ͳɹ�, �������ݿ�
		string strTokenID = TokenId["tokenid"].asString();
		string strBody = Fast_Writer.write(BodyParam["body"]);
		ServerDB::GetInstance()->InsertPushMsg2DB(strTokenID, strBody);
		
		// ���շ��ص�����
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if (nRecvBytes == 0)
			{
				gvLog(LOG_WARNING, _T("Socket �Ѿ����رգ����͵�Pushmessage��"));
				gvLog(LOG_WARNING, S2T(strHttpPost).c_str());
				OutputDebugString(_T("Socket �Ѿ����ر�,���͵�Pushmessage��:\n"));
				OutputDebugString(S2T(strHttpPost).c_str());
				OutputDebugString(_T("\n"));
				break;
			} 
			else if(nRecvBytes == SOCKET_ERROR)
			{
				if (WSAESHUTDOWN ==  WSAGetLastError())
				{
					gvLog(LOG_WARNING, _T("recv���ش���WSAGetLastError���� WSAESHUTDOWN, ���͵�PushMessage�ǣ�"));
					gvLog(LOG_WARNING, S2T(strHttpPost).c_str());
					OutputDebugString(_T("recv���ش���WSAGetLastError���� WSAESHUTDOWN���͵�PushMessage�ǣ�\n"));
					OutputDebugString(S2T(strHttpPost).c_str());
					OutputDebugString(_T("\n"));
					break;
				} 
				else if(WSAETIMEDOUT == WSAGetLastError())
				{
					OutputDebugString(_T("���ӳ�ʱ\n"));
					break;
				}

			}
			else if (nRecvBytes >0)
			{
				// ���ճɹ�
				string HttpBody = RemoveHttpHead(szRecvBuf);
				if(!HttpBody.empty())
				{
					if (NULL != m_pListener)
					{
						m_pListener->OnSuccessRecv(HttpBody);
					}
					//ȥ��Httpͷ�ɹ�
					Json::Value RecvRoot;
					Json::Reader Reader;
					if (Reader.parse(HttpBody, RecvRoot))
					{
						string strRet = RecvRoot["ret"].asString();
						if (strRet=="0")
						{
							OutputDebugString(_T("PushMessage�����������سɹ�\n"));
							return TRUE;

						}
						else if (strRet == "1001")
						{
							// �������ܾ�����
							gvLog(LOG_WARNING,_T("PushMessage����ʧ�ܣ��������ܾ�����"));
							OutputDebugString(_T("PushMessage����ʧ�ܣ��������ܾ�����\n"));
							return FALSE;
						}
						else if(strRet == "1101")
						{

							// ���ݸ�ʽ����ȷ
							gvLog(LOG_WARNING,_T("PushMessage����ʧ�ܣ����ݸ�ʽ����ȷ"));
							gvLog(LOG_WARNING,S2T(strHttpPost).c_str());
							OutputDebugString(_T("PushMessage����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
							OutputDebugString(S2T(strHttpPost).c_str());
							OutputDebugString(_T("\n"));
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

				break;
			}	
		}
		return FALSE;
	}

}


// �����̣߳��������ӵ�Push������
DWORD WINAPI CServerEndPoint::_ConnectionThread(LPVOID lParam)
{
	CServerEndPoint *pServerEndPoint = reinterpret_cast<CServerEndPoint*>(lParam);
	// ��λ���ӱ�־
	pServerEndPoint->m_bIsConnected = FALSE;
	// ��λ��ʱ��־
	pServerEndPoint->m_bIsTimeOut = FALSE;
	
	ASSERT(INVALID_SOCKET != pServerEndPoint->m_socket);
	
	if (0 != ::connect(pServerEndPoint->m_socket, (SOCKADDR*)&(pServerEndPoint->m_PushServerAddr),sizeof(pServerEndPoint->m_PushServerAddr) ))
	{
		// ����ʧ��
		int nErr = WSAGetLastError();
		return nErr;
	}
	else
	{
		// ���ӳɹ������������߳�
		pServerEndPoint->m_bIsConnected = TRUE;
		// ����Challenge
	//	pServerEndPoint->GetChallenge();
		// ע�������
	//	pServerEndPoint->LoginServer();
	//	HANDLE hWorkerThread = ::CreateThread(NULL, 0, _RecvThread, lParam, 0, NULL);
	//	CloseHandle(hWorkerThread);
		return TRUE;
	}	
}

// �������̣߳���Push���������н�����������Ϣ
DWORD WINAPI CServerEndPoint::_RecvThread(LPVOID lParam)
{
	CServerEndPoint *pServerEndPoint = reinterpret_cast<CServerEndPoint*>(lParam);
	const int nBufSize = 10240;
	char szBuffer[nBufSize] = {0};
	int nRecvSize = 0;
	while (TRUE)
	{
		nRecvSize = ::recv(pServerEndPoint->m_socket, szBuffer, nBufSize, 0);
		if (nRecvSize >0 )
		{
			// �յ�Push�������ķ�������Ϣ
			szBuffer[nRecvSize] = NULL;
			pServerEndPoint->RecvDataProc(szBuffer);
		} 
		else
		{
			OutputDebugString(_T("����ʧ�ܣ����ӻ��ѶϿ���\n"));
			break;
		}
	}
	return 0;
}
// ������յ�����Ϣ
// �����ݽ��з���
int CServerEndPoint::RecvDataProc(LPCSTR pszRecvBuf)
{
	return 0;
}
string CServerEndPoint::RemoveHttpHead(LPCSTR pszRecvBuf)
{
	string temp(pszRecvBuf);
	string::size_type nIndex =  temp.find("\r\n\r\n");
	if (string::npos != nIndex)
	{
		// �ҵ�
		nIndex +=3;		//�Ƶ����
		string HttpBody = temp.substr(nIndex, (temp.length() - nIndex +1));	
		return HttpBody;
	}
	else
		return temp;
	
}