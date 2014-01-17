#include "ClientEndPoint.h"
#include <Mstcpip.h>
#include "ClientQueue.h"

static int g_challengesuccess = 0;
static int g_registersuccess = 0;
static int g_pushmessagesuccess = 0;
LONG volatile g_nPushMsgNum = 0; 	

// 根据错误代码返回对应的错误信息
TSTRING GetErrCodeInfo(DWORD dwErrCode)
{
	TSTRING strErrMessage;
	HLOCAL hlocal = NULL;			// Buffer that gets the error message string
	//DWORD dwLanguageId = MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED);	
	//DWORD dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	// 获得错误代码的相关描述信息
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
		OutputDebugString(_T("错误代码找不到对应的信息\n"))	;
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

// 设置TokenID
void  CClientEndPoint::SetTokenID(const string& strTokenID)
{
	ASSERT(!strTokenID.empty());
	m_strTokenID = strTokenID;
}

// 设置SchoolID和SchoolKey
void  CClientEndPoint::SetSchoolInfo(const string& strSchoolID, string strSchoolKey)
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

void CClientEndPoint::SetTimeThreshold(const TIME_THRESHOLD& Timehreshold)
{
	m_TimeShreshold = Timehreshold;
	m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOnlineIntervalTime,
		m_TimeShreshold.nMaxOnlineIntervalTime);
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
			OutputDebugString(_T("Socket 绑定失败\n"));
			gvLog(LOG_WARNING, _T("Socket绑定失败"));
			DisConnect2PushSRV();
			return FALSE;
		}

	} 
	// 加入TCP Keep-Alive 机制
	tcp_keepalive tcp_keepalive_param;
	tcp_keepalive_param.onoff = TRUE;
	tcp_keepalive_param.keepaliveinterval = 30000;			// 三十秒发送一次心跳
	tcp_keepalive_param.keepalivetime		= 60000;			// 六十秒内没收到则认为连接已经断开
	DWORD cbBytesReturned;
	if ( WSAIoctl(m_hsocket, SIO_KEEPALIVE_VALS,
		(LPVOID)&tcp_keepalive_param,
		sizeof(tcp_keepalive_param), 
		NULL, 0, (LPDWORD)&cbBytesReturned, NULL, NULL) == SOCKET_ERROR )
	{
		DWORD dwErrCode = WSAGetLastError();
		if (ERROR_IO_PENDING != dwErrCode )			// 是否成功发起重叠操作
		{
			OutputDebugString(_T("Socket 设置Keep-alive失败!\n"));
			gvLog(LOG_WARNING, _T("Socket 设置Keep-alive失败!\n") );
			return FALSE;
		}
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
		gvLog(LOG_WARNING, _T("Socket 连接到PushServer失败!\n") );
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

// 自动切换状态
DWORD CClientEndPoint::AutoDecideStatus(DWORD nMilliSec)
{
	m_nCurrentTime+= nMilliSec;
	switch (m_CurrentStatus)
	{
	case offline:
		{
			if (m_nCurrentTime >= m_nTime2SwithStatus)
			{
				// 到达切换状态的时间，将自己放入到队列中，等待Connect
				m_CurrentStatus = login;
				ClientConnQueue::GetInstance()->Push2Queue(this);
			}
		}
		break;
	case login:
		{
			if (m_nCurrentTime >60000)
			{
				// 如果两分钟之后还没有登陆上，则认
				// 为登陆失败，放入Disconn队列等候处理
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

BOOL CClientEndPoint::SwithStatus2Online()				// 上线（连接到PushServer）
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
		// 判断时间是否到上线时间
		if (m_nCurrentTime >= m_nTime2SwithStatus)
		{
			// 到达切换时间
			if (Connect2PushSRV())
			{
				// 连接成功
				return TRUE;
			} 
			else
			{
				// 连接失败
				DisConnect2PushSRV();
				return FALSE;
			}
		}
		else
		{
			// 还没到达切换的时间
			return FALSE;
		}
		// 如果是
		if (Connect2PushSRV())
		{
			// 连接到PushServer成功
			AsyncSendGetChallenge();
			// 重新计时
			return TRUE;
		}
		else
		{
			DisConnect2PushSRV();
			return FALSE;
		}
		// 否
	}
	return TRUE;
}

BOOL CClientEndPoint::SwithStatus2Offline()				// 下线（断开连接）
{
	if (m_CurrentStatus == offline)
	{
		return TRUE;
	} 
	else
	{
		// 判断是否时间到下线时间
		if (DisConnect2PushSRV())
		{
			// 重新计时
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
	JsonParam["SocketHandle"] = Json::Value((UINT)m_hsocket);
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
	DWORD	flags = 0;								//标志
	DWORD	dwSendBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_GETCHALLENGE;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// 将要发的内容复制到Buffer
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
		if (ERROR_IO_PENDING != WSAGetLastError())			// 是否成功发起重叠操作
		{
			OutputDebugString(_T("getchallenge 发送失败!\n"));
			return FALSE;
		}
	}
	TCHAR szDebugstr[256] = {0};
	_stprintf_s(szDebugstr, _countof(szDebugstr),
		_T("Socket : %d  发起GetChallenge，TokenID是：%s\n"), (UINT)(m_hsocket), S2T(m_strTokenID).c_str());
	gvLog(LOG_WARNING, szDebugstr);
	return TRUE;

	/*

	))
	{
	}
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
	return TRUE;*/

}


// WsaSend Getchallenge返回，判断是否成功，或成功则进行下一步操作
void CClientEndPoint::OnSendGetChallengeRet(DWORD dwIOBytesSize)
{
	// 判断Getchallenge返回是否成功
	if (dwIOBytesSize >0 )
	{
		// 发送成功，执行下一步操作
		AsyncRecvGetChallenge();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send操作返回失败
		gvLog(LOG_WARNING, _T("发送GetChallenge 返回为0，连接可能被关闭"));
		OutputDebugString(_T("发送GetChallenge 返回为0\n"));
	}
	else if (SOCKET_ERROR == dwIOBytesSize )
	{
		// 出错
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
	// 接收服务器返回的Challenge
	DWORD	flags = 0;								//标志
	DWORD	dwRecvBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_GETCHALLENGE;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;
	
	// 读取数据
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

// WsaRecv Getchallenge返回，判断是否成功，或成功则进行下一步操作
void CClientEndPoint::OnRecvGetChallengeRet(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize>0)
	{

		TCHAR szDebugstr[256] = {0};
		_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  GetChallenge返回\n"), (UINT)(m_hsocket) ) ;
		gvLog(LOG_WARNING, szDebugstr);

		// 接收成功
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);
		// 解析收到的数据
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("请求getchallenge后，服务器返回数据为空!"));
			OutputDebugString(_T("请求getchallenge后，服务器返回数据为空\n"));
			//return FALSE;
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
					g_challengesuccess +=1;
					m_strChallenge = Root["chanllenge"].asString();
					if (!m_strChallenge.empty())
					{
						OutputDebugString(_T("chanllenge 返回成功!\n"));
					//	int nRet = shutdown(m_hsocket, SD_SEND);
					//	StopSendMsg2PushSRV();

						// 2013-6-18   Added by Sanwen	
						if (GetChallenge == m_TestType)
						{
							// 中断
							DisConnect2PushSRV();
						} 
						else
						{
							// 继续操作
							AsyncSendRegisterTokenID();
						}
						//						return TRUE;
					}
					else
					{
						gvLog(LOG_WARNING,_T("chanllenge返回值为空!"));
						OutputDebugString(_T("chanllenge返回值为空!\n"));
						//						return FALSE;
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
				//				return FALSE;
			}
		}
		else
		{
			//收到的Buffer格式错误，解析失败
			gvLog(LOG_WARNING,_T("收到的Buffer格式错误，去除Http头失败"));
			OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
			//			return FALSE;
		}

	}
	else if ( 0 == dwIOBytesSize )
	{
		// Socket被关闭
		OutputDebugString(_T("RecvGetChallenge 返回0\n"));

	}
	else if( SOCKET_ERROR == dwIOBytesSize)
	{
		// 出错
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}
}

// 注册TokenID
BOOL CClientEndPoint::AsyncSendRegisterTokenID()
{
	if (!m_bIsConnected)
	{
		
		return FALSE;
	}
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
	// char schoolId[64] = {0};
	// strcpy_s(schoolId, _countof(schoolId), m_strSchoolID.c_str());
	string Appid = Md5HexString(m_strSchoolID.c_str());

	// CheckCode
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

	// 构造要发送的内容
	strHttpPost += strHttpHead;
	strHttpPost += strHttpBody;


	// 发送给Push服务器
	DWORD	flags = 0;								//标志
	DWORD	dwSendBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_REGISTER_TOKENID;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// 将要发的内容复制到Buffer
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
		if (ERROR_IO_PENDING != WSAGetLastError())			// 成功发起重叠操作
		{
			OutputDebugString(_T("RegisterTokenID 发送失败!\n"));
			return FALSE;
		}
	}

	TCHAR szDebugstr[256] = {0};
	_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  发起RegisterTokenID\n"), (UINT)(m_hsocket) ) ;
	gvLog(LOG_WARNING, szDebugstr);
	return TRUE;

	/*


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
	return TRUE;*/

}

void CClientEndPoint::OnSendRegisterTokenIDRet(DWORD dwIOBytesSize)
{
	// 判断Getchallenge返回是否成功
	if (dwIOBytesSize >0 )
	{
		// 发送成功，执行下一步操作
		AsyncRecvRegisterTokenID();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send操作返回失败
		OutputDebugString(_T("发送GetChallenge 返回失败\n"));
	}
	else if (SOCKET_ERROR == dwIOBytesSize )
	{
		// 出错
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
	// 接收服务器返回的Challenge
	DWORD	flags = 0;								//标志
	DWORD	dwRecvBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_REGESTER_TOKENID;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;

	// 读取数据
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
		_stprintf_s(szDebugstr, _countof(szDebugstr), _T("Socket : %d  RegisterTokenID返回\n"), (UINT)(m_hsocket) ) ;
		gvLog(LOG_WARNING, szDebugstr);
		// 接收成功
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);

		// 解析收到的数据
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("请求RegisterTokenID 后，服务器返回数据为空!"));
			OutputDebugString(_T("请求RegisterTokenID 后，服务器返回数据为空\n"));
			//	return FALSE;
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
					g_registersuccess +=1;
					OutputDebugString(_T("RegisterTokenID，服务器返回成功\n"));

					// 2013-6-18   Added by Sanwen	
					if (RegisterToken == m_TestType)
					{
						// 中断
						DisConnect2PushSRV();
					} 
					else
					{
						// 继续操作
						AsyncSendPushMessage();
					}
					//	return TRUE;

				}
				else if (strRet == "1001")
				{
					// 服务器拒绝访问
					gvLog(LOG_WARNING,_T("RegisterTokenID 返回失败，服务器拒绝访问"));
					OutputDebugString(_T("RegisterTokenID 返回失败，服务器拒绝访问\n"));
					//	return FALSE;
				}
				else if(strRet == "1101")
				{

					// 数据格式不正确
					gvLog(LOG_WARNING,_T("RegisterTokenID 返回失败，数据格式不正确"));
					OutputDebugString(_T("RegisterTokenID 返回失败，数据格式不正确\n"));
					SOCKADDR_IN sock_addr;
					int nLen = sizeof(SOCKADDR_IN);
					getsockname(m_hsocket,(SOCKADDR*)&sock_addr, &nLen);
					OutputDebugString(_T("本机端口是："));
					OutputDebugString(itot(ntohs(sock_addr.sin_port), 10).c_str());
					OutputDebugString(S2T(HttpBody).c_str());
					//		return FALSE;
				}
			} 
			else
			{
				// Json解析不到challenge
				OutputDebugString(_T("Json解析不到RegisterTokenID返回的值 \n"));
				//	return FALSE;
			}
		}
		else
		{
			//收到的Buffer格式错误，解析失败
			OutputDebugString(_T("收到的Buffer格式错误，去除Http头失败\n"));
			//	return FALSE;
		}
	}
	else if (dwIOBytesSize == 0)
	{
		// Socket被关闭
	}
	else
	{
		// 出错
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}

}


// 请求Push消息
BOOL CClientEndPoint::AsyncSendPushMessage()
{
	if (!m_bIsConnected)
	{
		return FALSE;
	}
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


	// 发送给Push服务器
	DWORD	flags = 0;								//标志
	DWORD	dwSendBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Send,sizeof(IO_OPERATION_DATA));
	m_IO_Send.optype = SEND_GETPUSHMESSAGE;
	strcpy_s(m_IO_Send.szBuf, DATA_BUF_SIZE,strHttpPost.c_str());		// 将要发的内容复制到Buffer
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
		if (ERROR_IO_PENDING != WSAGetLastError())			// 成功发起重叠操作
		{
			OutputDebugString(_T("RegisterTokenID 发送失败!\n"));
			return FALSE;
		}
	}
	//StopSendMsg2PushSRV();
	return TRUE;


	/*
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
	}	*/

	//return TRUE;
}

void CClientEndPoint::OnSendPushMessageRet(DWORD dwIOBytesSize)
{
	// 判断SendPushMessage返回是否成功
	if (dwIOBytesSize >0 )
	{
		// 发送成功，执行下一步操作
//	int nRet = shutdown(m_hsocket, SD_SEND);
	//	int nRet = shutdown(m_hsocket,SD_SEND);

		// SendPushMessage，状态由Login改为online
		m_CurrentStatus = online;
		m_nTime2SwithStatus = 1000*GenerateRandomTime(m_TimeShreshold.nMinOfflineIntervalTime,
			m_TimeShreshold.nMaxOfflineIntervalTime);
		m_nCurrentTime = 0;
		AsyncRecvPushMessage();
	}
	else if( 0== dwIOBytesSize )
	{
		// GetChallenge Send操作返回失败
		OutputDebugString(_T("发送GetChallenge 返回失败\n"));
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
	// 接收Recv PushMessage返回数据
	DWORD	flags = 0;								//标志
	DWORD	dwRecvBytes =0;					//发送字节数
	ZeroMemory(&m_IO_Recv, sizeof(IO_OPERATION_DATA));
	m_IO_Recv.optype = RECV_PUSHMESSAGE;
	WSABUF wsaBuf;
	wsaBuf.buf = m_IO_Recv.szBuf;
	wsaBuf.len = DATA_BUF_SIZE;

	// 读取数据
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
		// 接收成功
		char szRecvBuf[DATA_BUF_SIZE] = {0};
		strcpy_s(szRecvBuf, DATA_BUF_SIZE, pIO_Data->szBuf);

		// 解析收到的数据
		//	PSTR pRecvBuf = szRecvBuf;
		if (0==strlen(szRecvBuf))
		{
			gvLog(LOG_WARNING, _T("请求getpushmessage后，服务器返回数据为空!"));
			OutputDebugString(_T("请求getpushmessage后，服务器返回数据为空\n"));
			//return FALSE;
		}
		string strRecvBuf(szRecvBuf);
		string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
	
		if (nIdex != string::npos)//返回成功
		{
			OutputDebugString(_T("请求getpushmessage后，服务器返回成功\n"));

			// 2013-6-18   Added by Sanwen	
			if (GetPushMsg == m_TestType)
			{
				// 中断
				DisConnect2PushSRV();
			} 
			else
			{
				// 继续操作
				// 接收Recv PushMessage返回数据
				if (m_bIsConnected)
				{
					DWORD	flags = 0;								//标志
					DWORD	dwRecvBytes =0;					//发送字节数
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
		// Socket被关闭
	}
	else
	{
		// 出错
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());
	}

}
// 不断收服务器发过来的消息
void CClientEndPoint::RecvPushMessage(PIO_OPERATION_DATA pIO_Data, DWORD dwIOBytesSize)
{
	if (dwIOBytesSize >0 )
	{
		g_pushmessagesuccess +=1;
		// 成功
		// 有数据
		string strRecvBuf(pIO_Data->szBuf, dwIOBytesSize);
		string strRecvBody;
		string::size_type nIdex = strRecvBuf.find("HTTP/1.0 200");
		if (nIdex != string::npos)//返回成功
		{
			// 去掉http头
			strRecvBody.clear();
			strRecvBody = RemoveHttpHead(strRecvBuf.c_str());
			OutputDebugString(_T("请求getpushmessage后，服务器返回成功\n"));
		}
		else
		{
			// 已经是Body，不用去Http头
			strRecvBody = strRecvBuf;
		}

		if (strRecvBody.length() > 0)
		{
			// 解析数据
			PushMsgList MsgList;
			ParsePushMessge(strRecvBody.c_str(), MsgList);

			// 存入数据库
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
			// 接收Recv PushMessage返回数据
			DWORD	flags = 0;								//标志
			DWORD	dwRecvBytes =0;					//发送字节数
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
		// 失败，连接可能被关闭

	} 
	else
	{
		// SOCKET ERROR
		// 出错
		int nErrCode = WSAGetLastError();
		TSTRING strErrMsg = GetErrCodeInfo(nErrCode);
		OutputDebugString(strErrMsg.c_str());

	}

}

// 根据nOpType，选择下一步的操作
DWORD CClientEndPoint::SeleteOperation(LPOVERLAPPED lpOverlapped, DWORD dwIOBytesSize)
{

	//获取扩展重叠结构指针
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

		// 循环接收PushMessage	
	case RECV_PUSHMESSAGE:
		RecvPushMessage(pIO, dwIOBytesSize);
		break;
	default:
		break;
	}
	return 1;
}

// 停止向PushServer发送消息
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
	//	// 接收服务器返回的Challenge
	//	DWORD	flags = 0;								//标志
	//	DWORD	dwRecvBytes =0;					//发送字节数
	//	WSABUF wsaBuf;
	//	wsaBuf.len = 0;
	//	wsaBuf.buf = NULL;


	//	// 读取数据
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

// 断开Push服务器的连接
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
// 开始进行数据接收测试
int  CClientEndPoint::StartRecvDataTest()
{
m_hTreadRecvData = ::CreateThread(NULL, 0, RecvDataThread, this, 0,0 );
//CloseHandle(m_hTreadRecvData);
return 0;
}*/



//// 停止数据接收测试
//int CClientEndPoint::StopRecvDataTest()
//{
//	m_bIsThreadshouldAbort = TRUE;
//	DisConnect2PushSRV();
//	WaitForSingleObject(m_hTreadRecvData, INFINITE);
//	CloseHandle(m_hTreadRecvData);
//	return 0;
//}

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

