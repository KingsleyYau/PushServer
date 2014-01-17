#include "stdafx.h"
#include "IOCPModel.h"
#include "ClientQueue.h"


CIOCPModel::CIOCPModel(void)
:m_hEventStopTest(NULL)
,m_hEventSafelyDelete(NULL)
,m_hIOCP(NULL)
,m_phWorkerThreads(NULL)
,m_dwServerIP(0) 
,m_nServerPort(0)
,m_pMainDlg(NULL)
,m_ClientArr(NULL)
,m_nOnlineNum(0)
//,m_nOnlineCount(0)
,m_bIsDeleteAllClient(FALSE)
{
	m_hEventStopTest = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hEventSafelyDelete = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CIOCPModel::~CIOCPModel(void)
{
	ReleaseResource();
}

// 初始化工作 
BOOL CIOCPModel::Initalize()
{
	if (CreateNewCompletionPort(0))
	{
		// IO完成端口创建成功
		// 创建工作线程
		m_nWorkerThreadNum = 2*GetNumOfProcessors() + 2;
		//m_nWorkerThreadNum = 1;
		m_phWorkerThreads = new HANDLE[m_nWorkerThreadNum];
		for (int i = 0; i<m_nWorkerThreadNum; ++i)
	//	for (int i = 0; i<1; ++i)
		{
			m_phWorkerThreads[i] = ::CreateThread(NULL, 0, _IOWorkerThread, this, 0, NULL);
		}
		return TRUE;
	}
	else
	{
		OutputDebugString(_T("I/O完成端口创建失败\n"));
		return FALSE;
	}
}

// 释放资源
BOOL CIOCPModel::ReleaseResource()
{
	DeleteAllClient();
	// 结束线程
	for (int i =0; i<m_nWorkerThreadNum; ++i)
	{
		// 向工作者线程发退出通知
		PostQueuedCompletionStatus(m_hIOCP, (DWORD)0, NULL,NULL);
		CloseHandle(m_phWorkerThreads[i]);
	}
	// 等待 工作者线程退出
	::WaitForMultipleObjects(m_nWorkerThreadNum, m_phWorkerThreads,TRUE, INFINITE);
	delete [] m_phWorkerThreads;

	// Add
	CloseHandle(m_hEventStopTest);
	CloseHandle(m_hEventSafelyDelete);
	// 关闭IO完成端口
	if (NULL != m_hIOCP)
	{
		CloseHandle(m_hIOCP);
	}
	return TRUE;
}
BOOL CIOCPModel::StartTest(DWORD dwServerIP, UINT nServerPort,
										  string strPreTokenID, UINT TokenID_StartNum,
										  UINT nClientNum, const TIME_THRESHOLD &tm_shreshold)
{
	m_bIsDeleteAllClient = FALSE;
	m_dwServerIP = dwServerIP;
	m_nServerPort = nServerPort;
	m_strPreTokenID = strPreTokenID;
	m_nTokenID_StartNum = TokenID_StartNum;
	m_nClientNum = nClientNum;
	m_time_threshold = tm_shreshold;
	DeleteAllClient();
	ClientConnQueue::GetInstance()->Clear();
	ClientDisConnQueue::GetInstance()->Clear();
	ResetEvent(m_hEventStopTest);
	ResetEvent(m_hEventSafelyDelete);
	HANDLE hThreadStartTest = ::CreateThread(NULL, 0, _StartTestTrhead, this, 0, NULL);
	CloseHandle(hThreadStartTest);
	return TRUE;
}

void CIOCPModel::EndTest()
{
	SetEvent(m_hEventStopTest);		// 让三个线程自动退出
	HANDLE pHandle[2];
	pHandle[0] = m_hThreadTimer;
	pHandle[1] = m_hThreadConn2PushSrv;
	//pHandle[2] = m_hThreadDisConn;
	::WaitForMultipleObjects(2, pHandle, TRUE, INFINITE);
	CloseHandle(m_hThreadTimer);
	CloseHandle(m_hThreadConn2PushSrv);
	//CloseHandle(m_hThreadDisConn);

	for (int i =0; i < m_nClientNum; ++i)
	{
		//m_ClientArr[i].DisConnect2PushSRV();
	//	if (m_ClientArr[i].m_CurrentStatus == online)
	//	{
			m_ClientArr[i].StopSendMsg2PushSRV();
	//	}
	
	}

	WaitForSingleObject(m_hEventSafelyDelete, 10000);
	delete [] m_ClientArr;
	m_ClientArr = NULL;
}

UINT CIOCPModel::GetCurrentConnectNum()			// 获取当前的并发连接数
{
	//return m_nOnlineCount;
	return m_nOnlineNum;

}
UINT CIOCPModel::GetRecvPushMsgNum()				// 获取接收的PushMsg数
{
	return g_nPushMsgNum;
}

// 启动线程
DWORD WINAPI CIOCPModel::_StartTestTrhead(LPVOID lParam)
{
	
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	CClientEndPoint *ClientArr = new CClientEndPoint[pThis->m_nClientNum];
	pThis->m_ClientArr = ClientArr;
	// 先填好客户端的相关信息
	for (int i=0; i<pThis->m_nClientNum; ++i)
	{
		char szbuf[128] = {0};
		sprintf_s(szbuf, "%s%d",pThis->m_strPreTokenID.c_str(), pThis->m_nTokenID_StartNum + i );
		ClientArr[i].SetTokenID(szbuf);
		ClientArr[i].SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		ClientArr[i].SetPushSRVAddr(pThis->m_dwServerIP,pThis->m_nServerPort);
		ClientArr[i].SetTimeThreshold(pThis->m_time_threshold);
	}

	pThis->m_hThreadConn2PushSrv = ::CreateThread(NULL, 0, _Connect2PushSrv, lParam, 0, 0);
	pThis->m_hThreadDisConn = ::CreateThread(NULL, 0, _DisConnectPushSrv, lParam, 0, 0);
	pThis->m_hThreadTimer	 = ::CreateThread(NULL, 0, _CalcTimer, lParam, 0, 0);
	return TRUE;
}
DWORD WINAPI CIOCPModel::_CalcTimer(LPVOID lParam)
{
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	while( WAIT_OBJECT_0 != ::WaitForSingleObject(pThis->m_hEventStopTest, 0) )
	{
		for (int i =0; i<pThis->m_nClientNum; ++i)
		{
			pThis->m_ClientArr[i].AutoDecideStatus(1000);
		}
		Sleep(950);
	}
	return 0;
}

DWORD WINAPI CIOCPModel::_Connect2PushSrv(LPVOID lParam)
{
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	// 不断检测Connect Queue是否为空，若不为空，则进行Connect操作
	ClientConnQueue *pConnQueue = ClientConnQueue::GetInstance();
	while( WAIT_OBJECT_0 != ::WaitForSingleObject(pThis->m_hEventStopTest, 0) )
	{
		CClientEndPoint *pClient =NULL;
			pClient = pConnQueue->PopFormQueue();
		if (NULL != pClient)
		{
			if (pClient->Connect2PushSRV())
			{
				// Connect 成功
				InterlockedIncrement((volatile LONG*)&pThis->m_nOnlineNum);
				/*pThis->m_csProtectCount.Lock();
				++pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();*/
				pThis->AssociateDeviceWithCompletionPort(pClient->GetSocketHandle(), (ULONG_PTR)pClient);
				pClient->AsyncSendGetChallenge();
			}
			else
			{
				// Connect 失败

			}				
		} 
	}

	return 0;
}
DWORD WINAPI CIOCPModel::_DisConnectPushSrv(LPVOID lParam)
{
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	// 不断检测DisConnect Queue是否为空，若不为空，则进行Disconn操作
	ClientDisConnQueue *pDisConnQueue = ClientDisConnQueue::GetInstance();
//	while( WAIT_OBJECT_0 != ::WaitForSingleObject(pThis->m_hEventStopTest, 0) )
	while(TRUE)
	{
		CClientEndPoint *pClient =NULL;
		pClient = pDisConnQueue->PopFormQueue();
		if (NULL != pClient)
		{
			if (pClient->DisConnect2PushSRV())
			{
				InterlockedDecrement((volatile LONG*)&pThis->m_nOnlineNum);
				if (WAIT_OBJECT_0 == ::WaitForSingleObject(pThis->m_hEventStopTest, 0) 
					&& pThis->m_nOnlineNum == 0)
				{
					SetEvent(pThis->m_hEventSafelyDelete);
					break;
				}
				if (pThis->m_nOnlineNum <0)
				{
					OutputDebugString(_T("错误\n"));
				}
	/*			pThis->m_csProtectCount.Lock();
				--pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();*/
			}
			
		} 
	}
	return 0;
}


/*

DWORD WINAPI CIOCPModel::_Connect2PushSrv(LPVOID lParam)
{
	
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	CClientEndPoint *ClientArr = new CClientEndPoint[pThis->m_nClientNum];

	pThis->m_ClientArr = ClientArr;
	
// 先设置客户端的相关信息

	for (int i =0; i<pThis->m_nClientNum; ++i)
	{
		char szbuf[128] = {0};
		sprintf_s(szbuf, "%s%d",pThis->m_strPreTokenID.c_str(), pThis->m_nTokenID_StartNum + i );
		ClientArr[i].SetTokenID(szbuf);
		ClientArr[i].SetSchoolInfo("724", "caEARyTTu4724BTIISABQNPbd7Rx22KU");
		ClientArr[i].SetPushSRVAddr(pThis->m_dwServerIP,pThis->m_nServerPort);
		ClientArr[i].SetTimeThreshold(pThis->m_time_threshold);
	}

	DWORD nStartTime = GetTickCount();

////////////////加入并行处理////////////////////////////////////
#pragma  omp parallel sections
	{
#pragma omp section
		for (int i = 0; i < pThis->m_nClientNum/2; ++i)
		{
			if (ClientArr[i].Connect2PushSRV())
			{
				// 连接成功，则加入数组和 I/O完成端口
					pThis->AddClient2IOCP(&ClientArr[i]);
					
			}
		}
#pragma omp section
		for (int j = pThis->m_nClientNum/2; j < pThis->m_nClientNum; ++j)
		{
			if (ClientArr[j].Connect2PushSRV())
			{
				// 连接成功，则加入数组和 I/O完成端口
				//#pragma omp critical 
					pThis->AddClient2IOCP(&ClientArr[j]);

			}
		}

	}
//////////////////////////////////////////////////////////////////////////
	//for (int i =0; i<pThis->m_nClientNum; ++i)
	//{
	//	if (ClientArr[i].Connect2PushSRV())
	//	{
	//		// 连接成功，则加入数组和 I/O完成端口

	//		{

	//			 pThis->m_arrayClientEndPoint.Add(&ClientArr[i]);
	//		//	pThis->m_arrayClientEndPoint.push_back(&ClientArr[i]);
	//			pThis->AssociateDeviceWithCompletionPort(ClientArr[i].GetSocketHandle(), (ULONG_PTR)&ClientArr[i]);
	//		}

	//	}
	//
	//}
	DWORD nSpendTime = GetTickCount() - nStartTime;
	TCHAR szBuf[24] = {0};
	_stprintf(szBuf, _T("Connect 耗时: %d"),nSpendTime);
	::MessageBox(NULL, szBuf, NULL, MB_OK);

	for (int i =0; i<pThis->m_arrayClientEndPoint.GetSize(); ++i)
	{
		pThis->m_arrayClientEndPoint.GetAt(i)->AsyncSendGetChallenge();
	}
	
	return TRUE;
}*/


// IO 工作线程
DWORD WINAPI CIOCPModel::_IOWorkerThread(LPVOID lParam)
{
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	HANDLE hComPort = pThis->m_hIOCP;

	DWORD dwIOBytesSize;
	CClientEndPoint *pClient;
	LPOVERLAPPED lpOverlapped;
	BOOL bExitThread = FALSE;
	
	while(!bExitThread)
	{
		dwIOBytesSize = -1;
		lpOverlapped = NULL;
		pClient = NULL;
		
		// 等待 I/O 操作结果
		BOOL bIORet = GetQueuedCompletionStatus(hComPort,
																&dwIOBytesSize,
																(LPDWORD)&pClient,
																&lpOverlapped,
																INFINITE);
	
		DWORD dwErr= GetLastError();

		if (bIORet)
		{
			// I/O请求完成
			if (NULL != lpOverlapped)
			{
				if (dwIOBytesSize > 0)
				{
					// 有数据
					pClient->SeleteOperation(lpOverlapped, dwIOBytesSize);
				} 
				else if (dwIOBytesSize ==0)
				{
					if (dwErr == 0)
					{
						OutputDebugString(_T("Socket调用了 shutdown\n"));		
						TSTRING strErrInfo = GetErrCodeInfo(dwErr);
						OutputDebugString(strErrInfo.c_str());
						ClientDisConnQueue::GetInstance()->Push2Queue(pClient);
					} 
					else if(dwErr == 997)
					{
						// 没有数据, 可能是调用了shutdown
						OutputDebugString(_T("Socket调用了 shutdown\n"));		
						TSTRING strErrInfo = GetErrCodeInfo(dwErr);
						OutputDebugString(strErrInfo.c_str());
						// 将对象放入Disconn队列中，等待CloseSocket
						ClientDisConnQueue::GetInstance()->Push2Queue(pClient);
					}

				}
				else
				{
					OutputDebugString(_T("dwIOBytesSize 为-1\n"));
				}
			} 
			else
			{

				TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				OutputDebugString(strErrInfo.c_str());
				OutputDebugString(_T("GetQueuedCompletionStatus 返回 TRUE，但lpOverlapped为NULL\n"));
			}
		} 
		else
		{
			// I/O请求失败
			if (NULL != lpOverlapped)
			{
				if (dwIOBytesSize == 0)
				{
					// 可能是调用了CloseSocket
					OutputDebugString(_T("IO请求失败，IOBytesSize等于0，可能调用了CloseSocket\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
					ClientDisConnQueue::GetInstance()->Push2Queue(pClient);

				} 
				else if(dwIOBytesSize >0)
				{
					OutputDebugString(_T("IO请求失败，IOBytesSize大于0\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				}
				else
				{
					OutputDebugString(_T("IO请求失败，IOBytesSize小于0\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				}
			} 
			else
			{
				if (dwErr == WAIT_TIMEOUT)
				{
					// 超时
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				} 
				else
				{
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				}
			}
		}
		// ====判断线程是否需要退出=====
		if (NULL == dwIOBytesSize &&
			NULL == pClient &&
			NULL == lpOverlapped)
		{
			// 线程必须退出
			bExitThread = TRUE;		
		}
/*
		else
		{
			if (NULL == lpOverlapped)
			{
				OutputDebugString(_T("lpOverlapped 为NULL\n"));
			}
			// 线程继续运行
			if (bIORet &&
				0== dwIOBytesSize &&
				lpOverlapped != NULL)
			{
				pThis->m_csProtectCount.Lock();
				--pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();
				if(0 == pThis->m_nOnlineCount)
				{
					SetEvent(pThis->m_hEventSafelyDelete);
					OutputDebugString(_T("可以安全的删除Client数组了\n"));
				}
			}
			else if (ERROR_SUCCESS == bIORet &&
				dwIOBytesSize == 0 &&
				lpOverlapped != NULL)				//addb by Sanwen
			{
				// 有Socket 关闭
				//InterlockedDecrement(&pThis->m_nOnlineNum);
				pThis->m_csProtectCount.Lock();
				--pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();
				TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				OutputDebugString(strErrInfo.c_str());
				OutputDebugString(_T("有Socket 关闭\n"));
			//	if (0 == pThis->m_nOnlineNum)
				if(0 == pThis->m_nOnlineCount)
				{
					SetEvent(pThis->m_hEventSafelyDelete);
					OutputDebugString(_T("可以安全的删除Client数组了\n"));
				}

				//if ( !pThis->m_bIsDeleteAllClient)
				//{
				//	// 说明是连接是自动断开的
				//	TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				//	OutputDebugString(strErrInfo.c_str());
				//	OutputDebugString(_T("有Socket 关闭\n"));
				//	pThis->DeleteClient(pClient);
				//}

			}
			else
			{
				// I/O操作成功 
				pClient->SeleteOperation(lpOverlapped, dwIOBytesSize);

			}
		}*/


	}

	return 0;
}
// 创建一个新的I/O完成端口
BOOL CIOCPModel::CreateNewCompletionPort(DWORD NumberOfConcurrentThreads)
{
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
										NULL, 0, NumberOfConcurrentThreads);
	if (m_hIOCP)
	{
		return TRUE;
	} 
	else
	{
		return FALSE;
	}
}

// 将Connect成功的Socket关联到完成端口
BOOL CIOCPModel::AssociateDeviceWithCompletionPort(HANDLE hDevice, ULONG_PTR CompletionKey)
{
	ASSERT(NULL != m_hIOCP);
	HANDLE hRet = CreateIoCompletionPort(hDevice, m_hIOCP, CompletionKey, 0);
	return(hRet == m_hIOCP);	
}

// 获取本机处理器个数
int CIOCPModel::GetNumOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

//void  CIOCPModel::AddClient2IOCP(CClientEndPoint* pClient)
//{
//	m_csAddClient.Lock();
//	//m_arrayClientEndPoint.Add(pClient);
//	AssociateDeviceWithCompletionPort(pClient->GetSocketHandle(), (ULONG_PTR)pClient);
//	m_csAddClient.Unlock();
//}
//void CIOCPModel::DeleteClient(CClientEndPoint *pClient)						// 删除某个客户端
//{
//	if (m_arrayClientEndPoint.IsEmpty())
//	{
//		return;
//	}
//	m_csDeleteClient.Lock();
//	for (int i = 0; i<m_arrayClientEndPoint.GetCount(); ++i)
//	{
//		if (pClient == m_arrayClientEndPoint.GetAt(i))
//		{
//			CClientEndPoint *p = m_arrayClientEndPoint.GetAt(i);
//			m_arrayClientEndPoint.RemoveAt(i);
//			// delete p;
//			// p = NULL;
//			break;
//		}
//	}
//	m_csDeleteClient.Unlock();
//}

void CIOCPModel::DeleteAllClient()
{
	m_csDeleteClient.Lock();
	delete []m_ClientArr;
	m_ClientArr = NULL;
	m_csDeleteClient.Unlock();

}