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
////设置SchoolID和SchoolKey
//void CServerEndPoint:: SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey)
//{
//	ASSERT(!strSchoolID.empty());
//	ASSERT(!strSchoolKey.empty());
//	this->m_strSchoolID = strSchoolID;
//	this->m_strSchoolKey = strSchoolKey;	
//}


// 设置回调
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
// 取消回调
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



// 设置AppID和AppKey
void CServerEndPoint::SetAppInfo(const string& strAppID, const string& strAppKey)
{
	ASSERT(!strAppID.empty());
	ASSERT(!strAppKey.empty());
	this->m_strAppID = strAppID;
	this->m_strAppKey = strAppKey;	
}

// 设置TokenID
void CServerEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	this->m_strTokenID = strTokenID;
}

//连接到Push服务器
BOOL CServerEndPoint::Connect2PushSRV(DWORD dwServerIP, UINT nPort)
{
	if (INVALID_SOCKET == m_socket)
	{
		m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} 
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);

	// 复位连接标志
	m_bIsConnected = FALSE;
	// 复位超时标志
	m_bIsTimeOut = FALSE;

	if (0 != ::connect(m_socket, (SOCKADDR*)&(m_PushServerAddr),sizeof(m_PushServerAddr) ))
	{
		// 连接失败
		int nErr = WSAGetLastError();
		return FALSE;
	}
	else
	{
		SOCKADDR_IN LocalAddr;
		int namelen = sizeof(LocalAddr);
		getsockname(m_socket, (SOCKADDR*)&LocalAddr, &namelen);
		TCHAR szBuf[64] = {0};
		_stprintf(szBuf, _T("本机的端口为：%d \n"), ntohs(LocalAddr.sin_port));
		OutputDebugString(szBuf);
		// 连接成功，创建接收线程
		m_bIsConnected = TRUE;

		return TRUE;
	}	

}
BOOL CServerEndPoint::SetTimeOut(UINT nTimeOut)
{
	if (INVALID_SOCKET != m_socket && nTimeOut >0 )
	{
		// 设置发送超时
		if (SOCKET_ERROR == ::setsockopt(m_socket,
																SOL_SOCKET,
																SO_SNDTIMEO,
																(char*)(&nTimeOut),
																sizeof(nTimeOut) )  )
		{
			return FALSE;
		}
		// 设置接收超时
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
		gvLog(LOG_ERR_USER, _T("设置超时失败：Socket无效或超时参数错误!"));
		OutputDebugString(_T("设置超时失败：Socket无效或超时参数错误!!\n"));
		return FALSE;
	}
}

// 断开Push服务器的连接
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
		gvLog(LOG_WARNING, _T("断开Push服务器失败!"));
		OutputDebugString(_T("断开Push服务器失败！\n"));
		return FALSE;
	}
}

//请求Challenge
BOOL CServerEndPoint::GetChallenge()
{
	ASSERT(m_bIsConnected);
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// 构造Http Request Head

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

	// 构造Http Body
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;
	
	// 发给Push服务器
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("getchallenge 发送失败!"));
		OutputDebugString(_T("getchallenge 发送失败"));
		OutputDebugString(S2T(szTime).c_str());
		return FALSE;
	}
	else
	{
		// 发送成功
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("getchallenge 返回错误!"));
				OutputDebugString(_T("getchallenge 返回错误\n"));
				OutputDebugString(S2T(szTime).c_str());
				return FALSE;
			}
			else
			{
				// 接收成功
				break;
			}	
		}
		// 解析收到的数据
	//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("请求getchallenge后，服务器返回数据为空!"));
			OutputDebugString(_T("请求getchallenge后，服务器返回数据为空\n"));
			OutputDebugString(S2T(szTime).c_str());
			return FALSE;
		}
	string HttpBody = RemoveHttpHead(szRecvBuf);
		if(!HttpBody.empty())
		{
			//去除Http头成功
			Json::Value Root;
			Json::Reader Reader;
			if (Reader.parse(HttpBody.c_str(), Root))
			{
				string strRet =Root["ret"].asString();
				if (strRet == "0")
				{
					// 返回成功
					m_strChallenge = Root["chanllenge"].asString();
					if (!m_strChallenge.empty())
					{
						OutputDebugString(_T("chanllenge 返回成功!\n"));
						return TRUE;
					}
					else
					{
						gvLog(LOG_WARNING,_T("chanllenge返回值为空!"));
						OutputDebugString(_T("chanllenge返回值为空!\n"));
						return FALSE;
					}
				}
				else if (strRet == "1001")
				{
					// 服务器拒绝访问
					gvLog(LOG_WARNING,_T("getchallenge返回失败，服务器拒绝访问"));
					OutputDebugString(_T("getchallenge返回失败，服务器拒绝访问\n"));
				}
				else if(strRet == "1101")
				{
					// 数据格式不正确
					gvLog(LOG_WARNING,_T("getchallenge返回失败，数据格式不正确"));
					OutputDebugString(_T("getchallenge返回失败，数据格式不正确\n"));
				}

			} 
			else
			{
				// Json解析不到challenge
				OutputDebugString(_T("Json解析不到challenge \n"));
				return FALSE;
			}
		}
		else
		{
			//收到的Buffer格式错误，解析失败
			gvLog(LOG_WARNING,_T("收到的Buffer格式错误，去除Http头失败"));
			OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
			return FALSE;
		}
	}	
	return FALSE;
}
//注册Server(登陆Push服务器)
BOOL  CServerEndPoint::LoginServer()
{
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// 构造Http Request Head

	strHttpHead += "POST / HTTP/1.1\r\n";
	strHttpHead += "CHARSET: UTF-8\r\n";
	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";

	// 生成AppID 和CheckCode
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

	// 构造Http Body---Json
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// 发给Push服务器
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("RegisterServer 发送失败"));
		OutputDebugString(_T("RegisterServer 发送失败\n"));
		return FALSE;
	}
	else
	{
		// 发送成功
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("RegisterServer recv返回错误"));
				OutputDebugString(_T("RegisterServer recv返回错误\n"));
				return FALSE;
			}
			else
			{
				// 接收成功
				break;
			}	
		}
		// 解析收到的数据
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("请求RegisterServer后，服务器返回数据为空!"));
			OutputDebugString(_T("请求RegisterServer后，服务器返回数据为空\n"));
			return FALSE;
		}

		string HttpBody = RemoveHttpHead(szRecvBuf);
		if(!HttpBody.empty())
		{
			//去除Http头成功
			Json::Value RecvRoot;
			Json::Reader Reader;
			if (Reader.parse(HttpBody, RecvRoot))
			{
				string strRet = RecvRoot["ret"].asString();
				if (strRet=="0")
				{
					OutputDebugString(_T("RegisterServer，服务器返回成功\n"));
					return TRUE;
			
				}
				else if (strRet == "1001")
				{
					// 服务器拒绝访问
					gvLog(LOG_WARNING,_T("RegisterServer返回失败，服务器拒绝访问"));
					OutputDebugString(_T("RegisterServer返回失败，服务器拒绝访问\n"));
					return FALSE;
				}
				else if(strRet == "1101")
				{

					// 数据格式不正确
					gvLog(LOG_WARNING,_T("RegisterServer返回失败，数据格式不正确"));
					OutputDebugString(_T("RegisterServer返回失败，数据格式不正确\n"));
					return FALSE;
				}
			} 
			else
			{
				// Json解析不到challenge
				OutputDebugString(_T("Json解析不到challenge \n"));
				return FALSE;
			}
		}
		else
		{
			//收到的Buffer格式错误，解析失败
			OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
			return FALSE;
		}
	}	
	
	return TRUE;
}
// 发送错误的CheckCode
//BOOL CServerEndPoint::SendErrorCheckCode2PushSRV()
//{
//
//	string strHttpPost;
//	string strHttpHead;
//	string strHttpBody;
//	// 构造Http Request Head
//
//	strHttpHead += "POST / HTTP/1.1\r\n";
//	strHttpHead += "CHARSET: UTF-8\r\n";
//	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
//	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
//	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";
//
//	// 生成AppID 和CheckCode
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
//	// 构造Http Body---Json
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
//	// 构造要发送的内容
//	strHttpPost += strHttpHead;
//	strHttpPost += strHttpBody;
//
//
//	// 发给Push服务器
//	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);
//
//	if(SOCKET_ERROR == nSendBytes)
//	{
//		gvLog(LOG_WARNING, _T("ErrorCheckCode 发送失败"));
//		OutputDebugString(_T("ErrorCheckCode 发送失败\n"));
//		return FALSE;
//	}
//	else
//	{
//		// 发送成功
//		int nRecvBytes = SOCKET_ERROR;
//		char szRecvBuf[512] = {0};
//		while(SOCKET_ERROR == nRecvBytes )
//		{
//			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
//			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
//			{
//				gvLog(LOG_WARNING, _T("ErrorCheckCode 接收错误"));
//				OutputDebugString(_T("ErrorCheckCode 接收错误\n"));
//				return FALSE;
//			}
//			else
//			{
//				// 接收成功
//				break;
//			}	
//		}
//		// 解析收到的数据
//		//	PSTR pRecvBuf = szRecvBuf;
//		if (0==strlen(szRecvBuf))
//		{
//			gvLog(LOG_WARNING, _T("请求ErrorCheckCode后，服务器返回数据为空!"));
//			OutputDebugString(_T("请求ErrorCheckCode后，服务器返回数据为空\n"));
//			return FALSE;
//		}
//		string HttpBody = RemoveHttpHead(szRecvBuf);
//		if(!HttpBody.empty())
//		{
//			//去除Http头成功
//			Json::Value RecvRoot;
//			Json::Reader Reader;
//			if (Reader.parse(HttpBody, RecvRoot))
//			{
//				string strRet = RecvRoot["ret"].asString();
//				if (!strRet.empty())
//				{
//					if (strRet=="2001")
//					{
//						OutputDebugString(_T("发送ErrorCheckCode后，服务器返回CheckCode错误（正确）\n"));
//						return TRUE;
//					}
//					else if (strRet == "0")
//					{
//						gvLog(LOG_WARNING,_T("发送ErrorCheckCode后，服务器返回成功"));
//						OutputDebugString(_T("发送ErrorCheckCode后，服务器返回成功（有问题）\n"));
//						return TRUE;
//
//					}
//					else if (strRet == "1101")
//					{
//
//						// 数据格式不正确
//						gvLog(LOG_WARNING,_T("ErrorCheckCode返回失败，数据格式不正确"));
//						OutputDebugString(_T("ErrorCheckCode返回失败，数据格式不正确\n"));
//						return FALSE;
//
//					}
//					else if (strRet == "1001")
//					{
//						// 服务器拒绝访问
//						gvLog(LOG_WARNING,_T("ErrorCheckCode返回失败，服务器拒绝访问"));
//						OutputDebugString(_T("ErrorCheckCode返回失败，服务器拒绝访问\n"));
//						return FALSE;
//
//					}
//				}
//
//			} 
//			else
//			{
//				// Json解析不到RegisterServer 返回的数据
//				OutputDebugString(_T("Json解析不到RegisterServer 返回的数据 \n"));
//				return FALSE;
//			}
//		}
//		else
//		{
//			//收到的Buffer格式错误，解析失败
//			OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
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
	// 构造Http Request Head

	strHttpHead += "POST / HTTP/1.1\r\n";
	strHttpHead += "CHARSET: UTF-8\r\n";
	strHttpHead += "CONNECTION: KEEP-ALIVE\r\n";
	strHttpHead += "CONTENT-TYPE: TEXT/HTML\r\n";
	strHttpHead += "ACCEPT-ENCODING: GZIP\r\n";

	// 构造Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	Json::Reader Reader;
	Json::Value TokenIdArr;
	Json::Value TokenId;
	Json::Value TokenBody;
	
	if (!Reader.parse(strBody, TokenBody))
	{
		OutputDebugString(_T("Body格式错误，Json解析不了"));
		if (NULL !=m_pListener)
		{
			TSTRING strErr = _T("要发送的内容Body格式错误，Json解析不了，发送失败！");
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;

	// 发给Push服务器
	int nSendBytes = ::send(m_socket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		if (NULL != m_pListener)
		{
			m_pListener->OnErrorSend(strHttpPost);
		}
		OutputDebugString(_T("PushMessage 发送失败"));
		return FALSE;
	}
	else
	{
		if (NULL != m_pListener)
		{
			m_pListener->OnSuccessSend(strHttpPost);
		}
		// 发送成功, 先入数据库
		string strTokenID = TokenId["tokenid"].asString();
		string strBody = Fast_Writer.write(BodyParam["body"]);
		ServerDB::GetInstance()->InsertPushMsg2DB(strTokenID, strBody);
		
		// 接收返回的数据
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_socket, szRecvBuf, 512, 0);
			if (nRecvBytes == 0)
			{
				gvLog(LOG_WARNING, _T("Socket 已经被关闭，发送的Pushmessage是"));
				gvLog(LOG_WARNING, S2T(strHttpPost).c_str());
				OutputDebugString(_T("Socket 已经被关闭,发送的Pushmessage是:\n"));
				OutputDebugString(S2T(strHttpPost).c_str());
				OutputDebugString(_T("\n"));
				break;
			} 
			else if(nRecvBytes == SOCKET_ERROR)
			{
				if (WSAESHUTDOWN ==  WSAGetLastError())
				{
					gvLog(LOG_WARNING, _T("recv返回错误，WSAGetLastError返回 WSAESHUTDOWN, 发送的PushMessage是："));
					gvLog(LOG_WARNING, S2T(strHttpPost).c_str());
					OutputDebugString(_T("recv返回错误，WSAGetLastError返回 WSAESHUTDOWN发送的PushMessage是：\n"));
					OutputDebugString(S2T(strHttpPost).c_str());
					OutputDebugString(_T("\n"));
					break;
				} 
				else if(WSAETIMEDOUT == WSAGetLastError())
				{
					OutputDebugString(_T("连接超时\n"));
					break;
				}

			}
			else if (nRecvBytes >0)
			{
				// 接收成功
				string HttpBody = RemoveHttpHead(szRecvBuf);
				if(!HttpBody.empty())
				{
					if (NULL != m_pListener)
					{
						m_pListener->OnSuccessRecv(HttpBody);
					}
					//去除Http头成功
					Json::Value RecvRoot;
					Json::Reader Reader;
					if (Reader.parse(HttpBody, RecvRoot))
					{
						string strRet = RecvRoot["ret"].asString();
						if (strRet=="0")
						{
							OutputDebugString(_T("PushMessage，服务器返回成功\n"));
							return TRUE;

						}
						else if (strRet == "1001")
						{
							// 服务器拒绝访问
							gvLog(LOG_WARNING,_T("PushMessage返回失败，服务器拒绝访问"));
							OutputDebugString(_T("PushMessage返回失败，服务器拒绝访问\n"));
							return FALSE;
						}
						else if(strRet == "1101")
						{

							// 数据格式不正确
							gvLog(LOG_WARNING,_T("PushMessage返回失败，数据格式不正确"));
							gvLog(LOG_WARNING,S2T(strHttpPost).c_str());
							OutputDebugString(_T("PushMessage返回失败，数据格式不正确\n"));
							OutputDebugString(S2T(strHttpPost).c_str());
							OutputDebugString(_T("\n"));
							return FALSE;
						}
					} 
					else
					{
						// Json解析不到challenge
						OutputDebugString(_T("Json解析不到challenge \n"));
						return FALSE;
					}
				}
				else
				{
					//收到的Buffer格式错误，解析失败
					OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
					return FALSE;
				}

				break;
			}	
		}
		return FALSE;
	}

}


// 连接线程，用于连接到Push服务器
DWORD WINAPI CServerEndPoint::_ConnectionThread(LPVOID lParam)
{
	CServerEndPoint *pServerEndPoint = reinterpret_cast<CServerEndPoint*>(lParam);
	// 复位连接标志
	pServerEndPoint->m_bIsConnected = FALSE;
	// 复位超时标志
	pServerEndPoint->m_bIsTimeOut = FALSE;
	
	ASSERT(INVALID_SOCKET != pServerEndPoint->m_socket);
	
	if (0 != ::connect(pServerEndPoint->m_socket, (SOCKADDR*)&(pServerEndPoint->m_PushServerAddr),sizeof(pServerEndPoint->m_PushServerAddr) ))
	{
		// 连接失败
		int nErr = WSAGetLastError();
		return nErr;
	}
	else
	{
		// 连接成功，创建接收线程
		pServerEndPoint->m_bIsConnected = TRUE;
		// 请求Challenge
	//	pServerEndPoint->GetChallenge();
		// 注册服务器
	//	pServerEndPoint->LoginServer();
	//	HANDLE hWorkerThread = ::CreateThread(NULL, 0, _RecvThread, lParam, 0, NULL);
	//	CloseHandle(hWorkerThread);
		return TRUE;
	}	
}

// 工作者线程，与Push服务器进行交互，接收消息
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
			// 收到Push服务器的发来的消息
			szBuffer[nRecvSize] = NULL;
			pServerEndPoint->RecvDataProc(szBuffer);
		} 
		else
		{
			OutputDebugString(_T("接收失败，连接或已断开！\n"));
			break;
		}
	}
	return 0;
}
// 处理接收到的消息
// 对数据进行分析
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
		// 找到
		nIndex +=3;		//移到最后
		string HttpBody = temp.substr(nIndex, (temp.length() - nIndex +1));	
		return HttpBody;
	}
	else
		return temp;
	
}