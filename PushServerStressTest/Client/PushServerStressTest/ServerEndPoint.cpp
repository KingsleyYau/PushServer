#include "StdAfx.h"
#include "ServerEndPoint.h"
#include "LogFile.h"
#define DEFAULT_IP _T("127.0.0.1")

CServerEndPoint::CServerEndPoint(void):
m_bIsConnected(FALSE),
m_bIsTimeOut(FALSE),
m_socket(INVALID_SOCKET)
{
}

CServerEndPoint::~CServerEndPoint(void)
{
}
//设置SchoolID和SchoolKey
void CServerEndPoint:: SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey)
{
	ASSERT(!strSchoolID.empty());
	ASSERT(!strSchoolKey.empty());
	this->m_strSchoolID = strSchoolID;
	this->m_strSchoolKey = strSchoolKey;	
}

//连接到Push服务器
BOOL CServerEndPoint::Connect2PushSRV(DWORD dwServerIP, UINT nPort)
{
	if (INVALID_SOCKET == m_socket)
	{
		m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} 
	SetTimeOut(2000);
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
		// 连接成功，创建接收线程
		m_bIsConnected = TRUE;
		// 请求Challenge
		//	pServerEndPoint->GetChallenge();
		// 注册服务器
		//	pServerEndPoint->LoginServer();
		//	HANDLE hWorkerThread = ::CreateThread(NULL, 0, _RecvThread, lParam, 0, NULL);
		//	CloseHandle(hWorkerThread);
		return TRUE;
	}	

//	DWORD dwThreadID;
//	HANDLE hConnectThread = ::CreateThread(NULL, 0, _ConnectionThread, (LPVOID)this, 0,&dwThreadID);
	//CloseHandle(hConnectThread);
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
	int nRet = ::shutdown(m_socket,SD_BOTH);
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
	return TRUE;
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
	string Appid = Md5HexString(m_strSchoolID.c_str());
	
	//// CheckCode

	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
	string strTemp = m_strChallenge;
	strTemp += MD5SchoolKey;
	string CheckCode = Md5HexString(strTemp.c_str());

	// 构造Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	BodyParam["appid"] = Appid;
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
BOOL CServerEndPoint::SendErrorCheckCode2PushSRV()
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
	string Appid = Md5HexString(m_strSchoolID.c_str());

	//// CheckCode

	string MD5SchoolKey = Md5HexString(m_strSchoolKey.c_str());
	//string strTemp = m_strChallenge;
	//strTemp += MD5SchoolKey;
	string strTemp = MD5SchoolKey;
	string CheckCode = Md5HexString(strTemp.c_str());

	// 构造Http Body---Json
	Json::Value BodyParam;
	Json::FastWriter Fast_Writer;
	BodyParam["appid"] = Appid;
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
		gvLog(LOG_WARNING, _T("ErrorCheckCode 发送失败"));
		OutputDebugString(_T("ErrorCheckCode 发送失败\n"));
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
				gvLog(LOG_WARNING, _T("ErrorCheckCode 接收错误"));
				OutputDebugString(_T("ErrorCheckCode 接收错误\n"));
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
			gvLog(LOG_WARNING, _T("请求ErrorCheckCode后，服务器返回数据为空!"));
			OutputDebugString(_T("请求ErrorCheckCode后，服务器返回数据为空\n"));
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
				if (!strRet.empty())
				{
					if (strRet=="2001")
					{
						OutputDebugString(_T("发送ErrorCheckCode后，服务器返回CheckCode错误（正确）\n"));
						return TRUE;
					}
					else if (strRet == "0")
					{
						gvLog(LOG_WARNING,_T("发送ErrorCheckCode后，服务器返回成功"));
						OutputDebugString(_T("发送ErrorCheckCode后，服务器返回成功（有问题）\n"));
						return TRUE;

					}
					else if (strRet == "1101")
					{

						// 数据格式不正确
						gvLog(LOG_WARNING,_T("ErrorCheckCode返回失败，数据格式不正确"));
						OutputDebugString(_T("ErrorCheckCode返回失败，数据格式不正确\n"));
						return FALSE;

					}
					else if (strRet == "1001")
					{
						// 服务器拒绝访问
						gvLog(LOG_WARNING,_T("ErrorCheckCode返回失败，服务器拒绝访问"));
						OutputDebugString(_T("ErrorCheckCode返回失败，服务器拒绝访问\n"));
						return FALSE;

					}
				}

			} 
			else
			{
				// Json解析不到RegisterServer 返回的数据
				OutputDebugString(_T("Json解析不到RegisterServer 返回的数据 \n"));
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

BOOL CServerEndPoint::SendPushMessage(CString strTokenIDlist, CString strBody)
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
	Json::Value TokenIdArr;
	Json::Value TokenId;
	Json::Value TokenBody;
	
	
	TokenId["tokenid"] = Json::Value("62af11b2baed583c3ba110f00efeb7e1");
	TokenIdArr.append(TokenId);
	TokenBody["aps"] = Json::Value("adfasdfw");
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
		OutputDebugString(_T("PushMessage 发送失败"));
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
				gvLog(LOG_WARNING, _T("PushMessage recv返回错误"));
				OutputDebugString(_T("PushMessager recv返回错误\n"));
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
			gvLog(LOG_WARNING, _T("发送PushMessage 后，服务器返回数据为空!"));
			OutputDebugString(_T("发送PushMessage 后，服务器返回数据为空\n"));
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
					OutputDebugString(_T("PushMessage返回失败，数据格式不正确\n"));
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