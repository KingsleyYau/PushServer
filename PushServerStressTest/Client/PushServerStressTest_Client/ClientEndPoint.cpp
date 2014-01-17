#include "StdAfx.h"
#include "ClientDB.h"
#include "ClientEndPoint.h"


LPCSTR HttpHead = "POST / HTTP/1.1\r\n"
							  "CHARSET: UTF-8\r\n"
							  "CONNECTION: KEEP-ALIVE\r\n"
							  "CONTENT-TYPE: TEXT/HTML\r\n"
							  "ACCEPT-ENCODING: GZIP\r\n";

CClientEndPoint::CClientEndPoint(void):
m_bIsConnected(FALSE),
m_bIsTimeOut(FALSE),
m_hsocket(INVALID_SOCKET),
m_bIsThreadshouldAbort(FALSE)
{
	m_strRecvBuffer.clear();
}

CClientEndPoint::~CClientEndPoint(void)
{
}

// ����TokenID
void  CClientEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	m_strTokenID = strTokenID;
}

// ����SchoolID��SchoolKey
void  CClientEndPoint::SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey)
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
		m_hsocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
		return FALSE;
	}
	else
	{
		m_bIsConnected = TRUE;
		return TRUE;
	}	
	return TRUE;
}
// ����Challenge
BOOL CClientEndPoint::GetChallenge()
{
	ASSERT(m_bIsConnected);
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
	return TRUE;
}
// ע��TokenID
BOOL CClientEndPoint::RegisterTokenID()
{
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
	//char schoolId[64] = {0};
	//strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
	string Appid = Md5HexString(m_strSchoolID.c_str());

	//// CheckCode

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
	return TRUE;
}
// ���ʹ����CheckCode
BOOL CClientEndPoint::SendErrorCheckCode2PushSRV()
{
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// ����Http Request Head

	strHttpHead = HttpHead;

	// ����AppID ��CheckCode
	// MD5(SchoolID)-->AppID
	// MD5(TokenID + chanllenge + MD5(SchoolKey))-->CheckCode

	// AppID
	//char schoolId[64] = {0};
	//strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
	string Appid = Md5HexString(m_strSchoolID.c_str());

	//// CheckCode

	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
	//string strTemp = m_strChallenge;
	//strTemp += MD5SchoolKey;
	string strTemp = MD5SchoolKey;
	string CheckCode = Md5HexString(strTemp.c_str());

	// ����Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	BodyParam["appid"] = Appid;
	BodyParam["checkcode"] = CheckCode;
	BodyParam["model"] = "i9100";
	BodyParam["system"] = "Android2.2";
	BodyParam["appver"] = "1.2";
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


	// ����Push������
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("ErrorCheckCode ����ʧ��"));
		OutputDebugString(_T("ErrorCheckCode ����ʧ��\n"));
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
				gvLog(LOG_WARNING, _T("ErrorCheckCode ���մ���"));
				OutputDebugString(_T("ErrorCheckCode ���մ���\n"));
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
			gvLog(LOG_WARNING, _T("����ErrorCheckCode�󣬷�������������Ϊ��!"));
			OutputDebugString(_T("����ErrorCheckCode�󣬷�������������Ϊ��\n"));
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
				if (!strRet.empty())
				{
					if (strRet=="2001")
					{
						OutputDebugString(_T("����ErrorCheckCode�󣬷���������CheckCode������ȷ��\n"));
						return TRUE;
					}
					else if (strRet == "0")
					{
						gvLog(LOG_WARNING,_T("����ErrorCheckCode�󣬷��������سɹ�"));
						OutputDebugString(_T("����ErrorCheckCode�󣬷��������سɹ��������⣩\n"));
						return TRUE;

					}
					else if (strRet == "1101")
					{

						// ���ݸ�ʽ����ȷ
						gvLog(LOG_WARNING,_T("ErrorCheckCode����ʧ�ܣ����ݸ�ʽ����ȷ"));
						OutputDebugString(_T("ErrorCheckCode����ʧ�ܣ����ݸ�ʽ����ȷ\n"));
						return FALSE;

					}
					else if (strRet == "1001")
					{
						// �������ܾ�����
						gvLog(LOG_WARNING,_T("ErrorCheckCode����ʧ�ܣ��������ܾ�����"));
						OutputDebugString(_T("ErrorCheckCode����ʧ�ܣ��������ܾ�����\n"));
						return FALSE;

					}
				}

			} 
			else
			{
				// Json��������RegisterServer ���ص�����
				OutputDebugString(_T("Json��������RegisterServer ���ص����� \n"));
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
// ����Push��Ϣ
BOOL CClientEndPoint::GetPushMessage()
{

	ASSERT(m_bIsConnected);
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
	}	
	//return TRUE;
}


// �Ͽ�Push������������
BOOL CClientEndPoint::DisConnect2PushSRV()
{
	if (INVALID_SOCKET == m_hsocket)
	{
		return TRUE;
	} 
	else
	{
		int nRet = ::shutdown(m_hsocket,SD_BOTH);
		if (0 == nRet)
		{
			m_bIsConnected = FALSE;
			::closesocket(m_hsocket);
			m_hsocket = INVALID_SOCKET;
			return TRUE;
		} 
		else
		{
			//gvLog(LOG_WARNING, _T("�Ͽ�Push������ʧ��!"));
			OutputDebugString(_T("�Ͽ�Push������ʧ��!\n"));
			return FALSE;
		}
	}
}


// ��ʼ�������ݽ��ղ���
int  CClientEndPoint::StartRecvDataTest()
{
	m_hTreadRecvData = ::CreateThread(NULL, 0, RecvDataThread, this, 0,0 );
	//CloseHandle(m_hTreadRecvData);
	return 0;
}


// ֹͣ���ݽ��ղ���
int CClientEndPoint::StopRecvDataTest()
{
	m_bIsThreadshouldAbort = TRUE;
	DisConnect2PushSRV();
	WaitForSingleObject(m_hTreadRecvData, INFINITE);
	CloseHandle(m_hTreadRecvData);
	return 0;
}

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

	/* ����StartRecvDataTest�󣬿�һ���̣߳����ڽ���
	* PushServer�����������ݣ���д�뵽���ݿ� */
DWORD WINAPI CClientEndPoint::RecvDataThread(LPVOID lParam)
{
	CClientEndPoint* pthis = reinterpret_cast<CClientEndPoint*>(lParam);
	DWORD dwRetCode = 0;
	if (pthis->Connect2PushSRV() )
	{
		OutputDebugString(_T("Connect�ɹ�\n"));
		if (pthis->GetChallenge())
		{
			if (pthis->RegisterTokenID())
			{
				//pthis->SetTimeOut(15000);		// ���볬ʱ
				ASSERT(pthis->m_bIsConnected);
				string strHttpPost;
				string strHttpHead;
				string strHttpBody;

				// ����Http Request Head
				strHttpHead = HttpHead;

				//HOST
				char szHostInfoBuf[128] = {0};
				LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
				sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
					inet_ntoa(pthis->m_PushServerAddr.sin_addr), ntohs(pthis->m_PushServerAddr.sin_port));
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

				// ����Push������
				int nSendBytes = ::send(pthis->m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

				if(SOCKET_ERROR == nSendBytes)
				{
					gvLog(LOG_WARNING, _T("getpushmessage ����ʧ��!"));
					OutputDebugString(_T("getpushmessage ����ʧ��"));
					dwRetCode = FAIL_SET_PUSHMESSAGE;
				}
				else
				{
					// ���ͳɹ�
					int nRecvBytes = SOCKET_ERROR;
					char szRecvBuf[1024] = {0};
					while(!pthis->m_bIsThreadshouldAbort)
					{
						nRecvBytes = ::recv(pthis->m_hsocket, szRecvBuf, 1024, 0);
						if (nRecvBytes == 0)
						{
							dwRetCode = SOCKET_CLOSE;
							break;
						}
						else if(nRecvBytes == SOCKET_ERROR)
						{
							if (WSAESHUTDOWN == WSAGetLastError())
							{
								// Socket�Ѿ����ر� , �˳�
								dwRetCode = SOCKET_CLOSE;
								break;
							}
							else if (WSAETIMEDOUT == WSAGetLastError())
							{
								continue;
							}
						}
						else if(nRecvBytes>0)
						{
							// ������
							string strRecvBuf(szRecvBuf, nRecvBytes);
							string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
							if (nIdex != string::npos)//���سɹ�
							{
								// ȥ��httpͷ
								strRecvBuf.clear();
								strRecvBuf = pthis->RemoveHttpHead(szRecvBuf);
								pthis->SetTimeOut(3000);			// getpushmessage�ɹ�������һ�볬ʱ
								OutputDebugString(_T("����getpushmessage�󣬷��������سɹ�\n"));
							}
					
							if (strRecvBuf.length() > 0)
							{

								// ��������
								PushMsgList MsgList;
								pthis->ParsePushMessge(strRecvBuf.c_str(), MsgList);
								
								// �������ݿ�
								for (PushMsgList::iterator iter = MsgList.begin();
									iter != MsgList.end(); ++ iter)
								{
									ClientDB::GetInstance()->InsertPushMsg2DB(iter->strTokenID,iter->strBody);									
								}
							}
		
							ZeroMemory(szRecvBuf,sizeof(szRecvBuf));
							nRecvBytes = SOCKET_ERROR;
						}				
					}

				}	
			}
			else
			{
				dwRetCode =  FAIL_REGISTER_TOKENID;
			}
		}
		else
		{
			dwRetCode =  FAIL_GET_CHALLENGE;
		}
		
	}
	else
	{
		OutputDebugString(_T("Connect ʧ��\n"));
		dwRetCode = FAIL_CONNECT_PUSHSRV;
	}
	// pthis->DisConnect2PushSRV();
	return dwRetCode;
}