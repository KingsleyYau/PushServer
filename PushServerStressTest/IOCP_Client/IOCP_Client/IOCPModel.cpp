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

// ��ʼ������ 
BOOL CIOCPModel::Initalize()
{
	if (CreateNewCompletionPort(0))
	{
		// IO��ɶ˿ڴ����ɹ�
		// ���������߳�
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
		OutputDebugString(_T("I/O��ɶ˿ڴ���ʧ��\n"));
		return FALSE;
	}
}

// �ͷ���Դ
BOOL CIOCPModel::ReleaseResource()
{
	DeleteAllClient();
	// �����߳�
	for (int i =0; i<m_nWorkerThreadNum; ++i)
	{
		// �������̷߳��˳�֪ͨ
		PostQueuedCompletionStatus(m_hIOCP, (DWORD)0, NULL,NULL);
		CloseHandle(m_phWorkerThreads[i]);
	}
	// �ȴ� �������߳��˳�
	::WaitForMultipleObjects(m_nWorkerThreadNum, m_phWorkerThreads,TRUE, INFINITE);
	delete [] m_phWorkerThreads;

	// Add
	CloseHandle(m_hEventStopTest);
	CloseHandle(m_hEventSafelyDelete);
	// �ر�IO��ɶ˿�
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
	SetEvent(m_hEventStopTest);		// �������߳��Զ��˳�
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

UINT CIOCPModel::GetCurrentConnectNum()			// ��ȡ��ǰ�Ĳ���������
{
	//return m_nOnlineCount;
	return m_nOnlineNum;

}
UINT CIOCPModel::GetRecvPushMsgNum()				// ��ȡ���յ�PushMsg��
{
	return g_nPushMsgNum;
}

// �����߳�
DWORD WINAPI CIOCPModel::_StartTestTrhead(LPVOID lParam)
{
	
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	CClientEndPoint *ClientArr = new CClientEndPoint[pThis->m_nClientNum];
	pThis->m_ClientArr = ClientArr;
	// ����ÿͻ��˵������Ϣ
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
	// ���ϼ��Connect Queue�Ƿ�Ϊ�գ�����Ϊ�գ������Connect����
	ClientConnQueue *pConnQueue = ClientConnQueue::GetInstance();
	while( WAIT_OBJECT_0 != ::WaitForSingleObject(pThis->m_hEventStopTest, 0) )
	{
		CClientEndPoint *pClient =NULL;
			pClient = pConnQueue->PopFormQueue();
		if (NULL != pClient)
		{
			if (pClient->Connect2PushSRV())
			{
				// Connect �ɹ�
				InterlockedIncrement((volatile LONG*)&pThis->m_nOnlineNum);
				/*pThis->m_csProtectCount.Lock();
				++pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();*/
				pThis->AssociateDeviceWithCompletionPort(pClient->GetSocketHandle(), (ULONG_PTR)pClient);
				pClient->AsyncSendGetChallenge();
			}
			else
			{
				// Connect ʧ��

			}				
		} 
	}

	return 0;
}
DWORD WINAPI CIOCPModel::_DisConnectPushSrv(LPVOID lParam)
{
	CIOCPModel *pThis = reinterpret_cast<CIOCPModel*>(lParam);
	// ���ϼ��DisConnect Queue�Ƿ�Ϊ�գ�����Ϊ�գ������Disconn����
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
					OutputDebugString(_T("����\n"));
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
	
// �����ÿͻ��˵������Ϣ

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

////////////////���벢�д���////////////////////////////////////
#pragma  omp parallel sections
	{
#pragma omp section
		for (int i = 0; i < pThis->m_nClientNum/2; ++i)
		{
			if (ClientArr[i].Connect2PushSRV())
			{
				// ���ӳɹ������������� I/O��ɶ˿�
					pThis->AddClient2IOCP(&ClientArr[i]);
					
			}
		}
#pragma omp section
		for (int j = pThis->m_nClientNum/2; j < pThis->m_nClientNum; ++j)
		{
			if (ClientArr[j].Connect2PushSRV())
			{
				// ���ӳɹ������������� I/O��ɶ˿�
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
	//		// ���ӳɹ������������� I/O��ɶ˿�

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
	_stprintf(szBuf, _T("Connect ��ʱ: %d"),nSpendTime);
	::MessageBox(NULL, szBuf, NULL, MB_OK);

	for (int i =0; i<pThis->m_arrayClientEndPoint.GetSize(); ++i)
	{
		pThis->m_arrayClientEndPoint.GetAt(i)->AsyncSendGetChallenge();
	}
	
	return TRUE;
}*/


// IO �����߳�
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
		
		// �ȴ� I/O �������
		BOOL bIORet = GetQueuedCompletionStatus(hComPort,
																&dwIOBytesSize,
																(LPDWORD)&pClient,
																&lpOverlapped,
																INFINITE);
	
		DWORD dwErr= GetLastError();

		if (bIORet)
		{
			// I/O�������
			if (NULL != lpOverlapped)
			{
				if (dwIOBytesSize > 0)
				{
					// ������
					pClient->SeleteOperation(lpOverlapped, dwIOBytesSize);
				} 
				else if (dwIOBytesSize ==0)
				{
					if (dwErr == 0)
					{
						OutputDebugString(_T("Socket������ shutdown\n"));		
						TSTRING strErrInfo = GetErrCodeInfo(dwErr);
						OutputDebugString(strErrInfo.c_str());
						ClientDisConnQueue::GetInstance()->Push2Queue(pClient);
					} 
					else if(dwErr == 997)
					{
						// û������, �����ǵ�����shutdown
						OutputDebugString(_T("Socket������ shutdown\n"));		
						TSTRING strErrInfo = GetErrCodeInfo(dwErr);
						OutputDebugString(strErrInfo.c_str());
						// ���������Disconn�����У��ȴ�CloseSocket
						ClientDisConnQueue::GetInstance()->Push2Queue(pClient);
					}

				}
				else
				{
					OutputDebugString(_T("dwIOBytesSize Ϊ-1\n"));
				}
			} 
			else
			{

				TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				OutputDebugString(strErrInfo.c_str());
				OutputDebugString(_T("GetQueuedCompletionStatus ���� TRUE����lpOverlappedΪNULL\n"));
			}
		} 
		else
		{
			// I/O����ʧ��
			if (NULL != lpOverlapped)
			{
				if (dwIOBytesSize == 0)
				{
					// �����ǵ�����CloseSocket
					OutputDebugString(_T("IO����ʧ�ܣ�IOBytesSize����0�����ܵ�����CloseSocket\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
					ClientDisConnQueue::GetInstance()->Push2Queue(pClient);

				} 
				else if(dwIOBytesSize >0)
				{
					OutputDebugString(_T("IO����ʧ�ܣ�IOBytesSize����0\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				}
				else
				{
					OutputDebugString(_T("IO����ʧ�ܣ�IOBytesSizeС��0\n"));
					TSTRING strErrInfo = GetErrCodeInfo(dwErr);
					OutputDebugString(strErrInfo.c_str());
				}
			} 
			else
			{
				if (dwErr == WAIT_TIMEOUT)
				{
					// ��ʱ
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
		// ====�ж��߳��Ƿ���Ҫ�˳�=====
		if (NULL == dwIOBytesSize &&
			NULL == pClient &&
			NULL == lpOverlapped)
		{
			// �̱߳����˳�
			bExitThread = TRUE;		
		}
/*
		else
		{
			if (NULL == lpOverlapped)
			{
				OutputDebugString(_T("lpOverlapped ΪNULL\n"));
			}
			// �̼߳�������
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
					OutputDebugString(_T("���԰�ȫ��ɾ��Client������\n"));
				}
			}
			else if (ERROR_SUCCESS == bIORet &&
				dwIOBytesSize == 0 &&
				lpOverlapped != NULL)				//addb by Sanwen
			{
				// ��Socket �ر�
				//InterlockedDecrement(&pThis->m_nOnlineNum);
				pThis->m_csProtectCount.Lock();
				--pThis->m_nOnlineCount;
				pThis->m_csProtectCount.Unlock();
				TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				OutputDebugString(strErrInfo.c_str());
				OutputDebugString(_T("��Socket �ر�\n"));
			//	if (0 == pThis->m_nOnlineNum)
				if(0 == pThis->m_nOnlineCount)
				{
					SetEvent(pThis->m_hEventSafelyDelete);
					OutputDebugString(_T("���԰�ȫ��ɾ��Client������\n"));
				}

				//if ( !pThis->m_bIsDeleteAllClient)
				//{
				//	// ˵�����������Զ��Ͽ���
				//	TSTRING strErrInfo = GetErrCodeInfo(dwErr);
				//	OutputDebugString(strErrInfo.c_str());
				//	OutputDebugString(_T("��Socket �ر�\n"));
				//	pThis->DeleteClient(pClient);
				//}

			}
			else
			{
				// I/O�����ɹ� 
				pClient->SeleteOperation(lpOverlapped, dwIOBytesSize);

			}
		}*/


	}

	return 0;
}
// ����һ���µ�I/O��ɶ˿�
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

// ��Connect�ɹ���Socket��������ɶ˿�
BOOL CIOCPModel::AssociateDeviceWithCompletionPort(HANDLE hDevice, ULONG_PTR CompletionKey)
{
	ASSERT(NULL != m_hIOCP);
	HANDLE hRet = CreateIoCompletionPort(hDevice, m_hIOCP, CompletionKey, 0);
	return(hRet == m_hIOCP);	
}

// ��ȡ��������������
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
//void CIOCPModel::DeleteClient(CClientEndPoint *pClient)						// ɾ��ĳ���ͻ���
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