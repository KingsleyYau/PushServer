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

// 设置TokenID
void  CClientEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	m_strTokenID = strTokenID;
}

// 设置SchoolID和SchoolKey
void  CClientEndPoint::SetSchoolInfo(const string& strSchoolID, const string& strSchoolKey)
{
	ASSERT(!strSchoolID.empty());
	ASSERT(!strSchoolKey.empty());
	m_strSchoolID = strSchoolID;
	m_strSchoolKey = strSchoolKey;	
}
// 设置PushServer地址和IP
void CClientEndPoint::SetPushSRVAddr(DWORD dwServerIP, UINT nPort)
{
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);
}
// 连接到Push服务器
BOOL CClientEndPoint::Connect2PushSRV(DWORD dwServerIP, UINT nPort)
{
	m_PushServerAddr.sin_family = AF_INET;
	m_PushServerAddr.sin_addr.S_un.S_addr = htonl(dwServerIP);
	m_PushServerAddr.sin_port = htons(nPort);
	return Connect2PushSRV();
}

// 连接到PushServer
BOOL CClientEndPoint::Connect2PushSRV()
{
	if (INVALID_SOCKET == m_hsocket)
	{
		m_hsocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} 
	// 复位连接标志
	m_bIsConnected = FALSE;
	// 复位超时标志
	m_bIsTimeOut = FALSE;

	if (0 != ::connect(m_hsocket, (SOCKADDR*)&(m_PushServerAddr),sizeof(m_PushServerAddr) ))
	{
		// 连接失败
		int nErr = WSAGetLastError();	
		TCHAR szBuf[32] = {0};
		_stprintf_s(szBuf,_T("连接到服务器失败，错误号：%d"),nErr);
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
// 请求Challenge
BOOL CClientEndPoint::GetChallenge()
{
	ASSERT(m_bIsConnected);
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// 构造Http Request Head

	strHttpHead = HttpHead;

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// 构造Http Body
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;

	// 发给Push服务器
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

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
			nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
			if (SOCKET_ERROR == nRecvBytes)
			{
				// 接收失败
				int nErr = WSAGetLastError();
				TCHAR szBuf[128] = {0};
				TCHAR szTokenID[32] = {0};
				_stprintf_s(szBuf, _T("  Getchallenge 调用recv返回SOCKET_ERROR,TokenID为: %s 错误代码为：%d\n"), S2T(m_strTokenID),nErr);
				OutputDebugString(szBuf);
				return FALSE;
			}
			else if (nRecvBytes == 0)
			{
				TCHAR szBuf[128] = {0};
				TCHAR szTokenID[32] = {0};
				_stprintf_s(szBuf, _T("  Getchallenge 调用recv返回0, 连接被关闭，TokenID为: %s\n"), S2T(m_strTokenID));
				OutputDebugString(szBuf);
				return FALSE;
			}
			//if ( nRecvBytes == 0 ) 
			//{
			//	gvLog(LOG_WARNING, _T("getchallenge 返回错误!"));
			//	OutputDebugString(_T("getchallenge 返回错误\n"));
			//	return FALSE;
			//}
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
// 注册TokenID
BOOL CClientEndPoint::RegisterTokenID()
{
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// 构造Http Request Head

	strHttpHead = HttpHead;

	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// 生成AppID 和CheckCode
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

	// 构造Http Body---Json
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// 发给Push服务器
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("RegisterTokenID 发送失败"));
		OutputDebugString(_T("RegisterTokenID 发送失败\n"));
		return FALSE;
	}
	else
	{
		// 发送成功
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes )
		{
			nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("RegisterTokenID recv返回错误"));
				OutputDebugString(_T("RegisterTokenID recv返回错误\n"));
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
			gvLog(LOG_WARNING, _T("请求RegisterTokenID 后，服务器返回数据为空!"));
			OutputDebugString(_T("请求RegisterTokenID 后，服务器返回数据为空\n"));
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
					OutputDebugString(_T("RegisterTokenID，服务器返回成功\n"));
					return TRUE;

				}
				else if (strRet == "1001")
				{
					// 服务器拒绝访问
					gvLog(LOG_WARNING,_T("RegisterTokenID 返回失败，服务器拒绝访问"));
					OutputDebugString(_T("RegisterTokenID 返回失败，服务器拒绝访问\n"));
					return FALSE;
				}
				else if(strRet == "1101")
				{

					// 数据格式不正确
					gvLog(LOG_WARNING,_T("RegisterTokenID 返回失败，数据格式不正确"));
					OutputDebugString(_T("RegisterTokenID 返回失败，数据格式不正确\n"));
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
BOOL CClientEndPoint::SendErrorCheckCode2PushSRV()
{
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;
	// 构造Http Request Head

	strHttpHead = HttpHead;

	// 生成AppID 和CheckCode
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

	// 构造Http Body---Json
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// 发给Push服务器
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

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
			nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
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
// 请求Push消息
BOOL CClientEndPoint::GetPushMessage()
{

	ASSERT(m_bIsConnected);
	string strHttpPost;
	string strHttpHead;
	string strHttpBody;

	// 构造Http Request Head
	strHttpHead = HttpHead;
	
	//HOST
	char szHostInfoBuf[128] = {0};
	LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
	sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
		inet_ntoa(m_PushServerAddr.sin_addr), ntohs(m_PushServerAddr.sin_port));
	strHttpHead += szHostInfoBuf;

	// 构造Http Body
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;

	// 发给Push服务器
	int nSendBytes = ::send(m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

	if(SOCKET_ERROR == nSendBytes)
	{
		gvLog(LOG_WARNING, _T("getpushmessage 发送失败!"));
		OutputDebugString(_T("getpushmessage 发送失败"));
		return FALSE;
	}
	else
	{
		// 发送成功
		int nRecvBytes = SOCKET_ERROR;
		char szRecvBuf[512] = {0};
		while(SOCKET_ERROR == nRecvBytes)
		{
			nRecvBytes = ::recv(m_hsocket, szRecvBuf, 512, 0);
			if ( nRecvBytes == 0 || nRecvBytes == WSAECONNRESET ) 
			{
				gvLog(LOG_WARNING, _T("getpushmessage 返回错误!"));
				OutputDebugString(_T("getpushmessage 返回错误\n"));
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
			gvLog(LOG_WARNING, _T("请求getpushmessage后，服务器返回数据为空!"));
			OutputDebugString(_T("请求getpushmessage后，服务器返回数据为空\n"));
			return FALSE;
		}
		string strRecvBuf(szRecvBuf);
		string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");

		if (nIdex != string::npos)//返回成功
		{
			OutputDebugString(_T("请求getpushmessage后，服务器返回成功\n"));
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}	
	//return TRUE;
}


// 断开Push服务器的连接
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
			//gvLog(LOG_WARNING, _T("断开Push服务器失败!"));
			OutputDebugString(_T("断开Push服务器失败!\n"));
			return FALSE;
		}
	}
}


// 开始进行数据接收测试
int  CClientEndPoint::StartRecvDataTest()
{
	m_hTreadRecvData = ::CreateThread(NULL, 0, RecvDataThread, this, 0,0 );
	//CloseHandle(m_hTreadRecvData);
	return 0;
}


// 停止数据接收测试
int CClientEndPoint::StopRecvDataTest()
{
	m_bIsThreadshouldAbort = TRUE;
	DisConnect2PushSRV();
	WaitForSingleObject(m_hTreadRecvData, INFINITE);
	CloseHandle(m_hTreadRecvData);
	return 0;
}

// 设置超时
BOOL CClientEndPoint::SetTimeOut(UINT nTimeOut)
{
	if (INVALID_SOCKET != m_hsocket && nTimeOut >0 )
	{
		// 设置发送超时
		if (SOCKET_ERROR == ::setsockopt(m_hsocket,
			SOL_SOCKET,
			SO_SNDTIMEO,
			(char*)(&nTimeOut),
			sizeof(nTimeOut) )  )
		{
			return FALSE;
		}
		// 设置接收超时
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
		gvLog(LOG_ERR_USER, _T("设置超时失败：Socket无效或超时参数错误!"));
		OutputDebugString(_T("设置超时失败：Socket无效或超时参数错误!!\n"));
		return FALSE;
	}
}
string CClientEndPoint::RemoveHttpHead(LPCSTR pszRecvBuf)
{
	string temp(pszRecvBuf);
	string::size_type nIndex =  temp.find("\r\n\r\n");
	if (string::npos != nIndex)
	{
		// 找到
		nIndex +=4;		//移到最后
		string HttpBody = temp.substr(nIndex, (temp.length() - nIndex));	
		return HttpBody;
	}
	else
		return temp;
}

// 解析数据
BOOL CClientEndPoint::ParsePushMessge(LPCSTR pszRecvBuf, PushMsgList& PushMessageList)
{
	BOOL bResult = FALSE;

	// 把收到的数据加入队列
	m_strRecvBuffer += pszRecvBuf;

	CStringA strRecvBuf(m_strRecvBuffer.c_str());

	// 循环解析
	while(-1 != strRecvBuf.Find("Context-Length:") )
	{
		int nBeginPos = -1;
		int nEndPos = -1;
		nBeginPos = strRecvBuf.Find("Context-Length:");
		nBeginPos += 15;			//跳到Content-Length;之后
		nEndPos = strRecvBuf.Find("\r\n\r\n", nBeginPos);
		if (nEndPos != -1)
		{
			CStringA strJsonLength = strRecvBuf.Mid(nBeginPos, (nEndPos-nBeginPos));
			strJsonLength.Trim();
			int nJsonLength = atoi(strJsonLength);			//获得了Json数据段的长度
			nBeginPos = nEndPos + 4;
			nEndPos = nBeginPos + nJsonLength;
			CStringA strJson = strRecvBuf.Mid(nBeginPos, nJsonLength);

			// strJson是否接收完整
			if (strJson.GetLength() == nJsonLength)
			{
				// 接收成功, 将该段Json从Buffer中删去
				strRecvBuf.Delete(0, nEndPos);	

				// Json段完整，可以解析
				LPCSTR pszJson = strJson.GetBuffer();
				Json::Reader reader;
				Json::FastWriter Fast_Writer;
				Json::Value root;
				if (reader.parse(pszJson, root))
				{
					// 解析成功, 
					// 放入PushMsgList
					PUSHMESSAGE PushMsg;
					PushMsg.strTokenID = root["tokenid"].asString();
					PushMsg.strBody = Fast_Writer.write(root["body"]);
					PushMessageList.push_back(PushMsg);
					bResult = TRUE;
					strJson.ReleaseBuffer();
				}
				else
				{
					// 解析不成功，格式可能错误
					strJson.ReleaseBuffer();
					// 写入Log, PushMessage Json解析失败
					char szLog[512] = {0};
					sprintf_s(szLog, "收到的PushMessage 用Json解析失败 ：%s", pszJson);
					gvLog(LOG_WARNING, S2T(szLog).c_str());
					OutputDebugString(S2T(szLog).c_str());	
				}

			} 
			else
			{
				// Json段不完整，退出While循环，保存数据队列，
				// 下次有收到数据时再进行解析
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

	/* 调用StartRecvDataTest后，开一个线程，用于接收
	* PushServer发过来的数据，并写入到数据库 */
DWORD WINAPI CClientEndPoint::RecvDataThread(LPVOID lParam)
{
	CClientEndPoint* pthis = reinterpret_cast<CClientEndPoint*>(lParam);
	DWORD dwRetCode = 0;
	if (pthis->Connect2PushSRV() )
	{
		OutputDebugString(_T("Connect成功\n"));
		if (pthis->GetChallenge())
		{
			if (pthis->RegisterTokenID())
			{
				//pthis->SetTimeOut(15000);		// 三秒超时
				ASSERT(pthis->m_bIsConnected);
				string strHttpPost;
				string strHttpHead;
				string strHttpBody;

				// 构造Http Request Head
				strHttpHead = HttpHead;

				//HOST
				char szHostInfoBuf[128] = {0};
				LPCSTR pszHostInfo = "HOST:%s:%d\r\n";
				sprintf_s(szHostInfoBuf, _countof(szHostInfoBuf), pszHostInfo,
					inet_ntoa(pthis->m_PushServerAddr.sin_addr), ntohs(pthis->m_PushServerAddr.sin_port));
				strHttpHead += szHostInfoBuf;

				// 构造Http Body
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

				// 构造要发送的内容
				strHttpPost += strHttpHead;
				strHttpPost += strHttpBody;

				// 发给Push服务器
				int nSendBytes = ::send(pthis->m_hsocket, strHttpPost.c_str(), strHttpPost.length(), 0);

				if(SOCKET_ERROR == nSendBytes)
				{
					gvLog(LOG_WARNING, _T("getpushmessage 发送失败!"));
					OutputDebugString(_T("getpushmessage 发送失败"));
					dwRetCode = FAIL_SET_PUSHMESSAGE;
				}
				else
				{
					// 发送成功
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
								// Socket已经被关闭 , 退出
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
							// 有数据
							string strRecvBuf(szRecvBuf, nRecvBytes);
							string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
							if (nIdex != string::npos)//返回成功
							{
								// 去掉http头
								strRecvBuf.clear();
								strRecvBuf = pthis->RemoveHttpHead(szRecvBuf);
								pthis->SetTimeOut(3000);			// getpushmessage成功后，设置一秒超时
								OutputDebugString(_T("请求getpushmessage后，服务器返回成功\n"));
							}
					
							if (strRecvBuf.length() > 0)
							{

								// 解析数据
								PushMsgList MsgList;
								pthis->ParsePushMessge(strRecvBuf.c_str(), MsgList);
								
								// 存入数据库
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
		OutputDebugString(_T("Connect 失败\n"));
		dwRetCode = FAIL_CONNECT_PUSHSRV;
	}
	// pthis->DisConnect2PushSRV();
	return dwRetCode;
}