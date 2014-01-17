#pragma once
#include "ClientEndPoint.h"
#include <omp.h>
#include <string>
#include "afxmt.h"
using namespace std;
#define ID_TIMER_ONE_SEC 1133

class CIOCPModel
{
public:
	CIOCPModel(void);
	virtual ~CIOCPModel(void);
	
	BOOL Initalize();
	BOOL StartTest(DWORD dwServerIP, UINT nServerPort,
			string strPreTokenID, UINT TokenID_StartNum, UINT nClientNum,TestType nTestType,
			const TIME_THRESHOLD &tm_shreshold);
	void EndTest();
	UINT GetCurrentConnectNum();			// ��ȡ��ǰ�Ĳ���������
	UINT GetRecvPushMsgNum();				// ��ȡ���յ�PushMsg��

protected:
	void AddClient2IOCP(CClientEndPoint* pClient);
	void DeleteClient(CClientEndPoint *pClient);						// ɾ��ĳ���ͻ���
	void DeleteAllClient(void);												// ɾ�����пͻ���
	static DWORD WINAPI _CalcTimer(LPVOID lParam);						// ��ʱ�߳�
	static DWORD WINAPI _Connect2PushSrv(LPVOID lParam);			// ���������Ƿ��ж������о�����PushSRV
	static DWORD WINAPI _DisConnectPushSrv(LPVOID lParam);			// ���������Ƿ��ж������оͶϿ���PushSRV������
	static DWORD WINAPI _StartTestTrhead(LPVOID lParam);				// �����߳�
	static DWORD WINAPI _IOWorkerThread(LPVOID lParam);			// I/O�����߳�
	


	int GetNumOfProcessors();	
	// ����һ���µ�I/O��ɶ˿�
	BOOL CreateNewCompletionPort(DWORD NumberOfConcurrentThreads);
	// ��Connect�ɹ���Socket��������ɶ˿�
	BOOL AssociateDeviceWithCompletionPort(HANDLE hDevice, ULONG_PTR CompletionKey);

	// �ͷ���Դ
	BOOL ReleaseResource();

private:
	//HANDLE	 m_hShutdownEvent;						// ֪ͨ�߳��˳�������
	HANDLE	 m_hIOCP;										// I/O��ɶ˿ڵľ��
	HANDLE	 *m_phWorkerThreads;					// I/O �������߾��ָ��
	int				m_nWorkerThreadNum;					// ���ɵĹ������߳���

//==========================//
	HANDLE	m_hThreadTimer;
	HANDLE	 m_hThreadConn2PushSrv;				// Connect�߳̾��
	HANDLE	 m_hThreadDisConn;
	HANDLE	m_hEventStopTest;							// ���߳��˳����¼�
	HANDLE	m_hEventSafelyDelete;					// ��ȫdelete Client���ݵ��¼�

//======Client�����Ϣ����========//
	DWORD	m_dwServerIP;								// PushServer��IP
	UINT			m_nServerPort;								// PushServer�Ķ˿�
	string		m_strPreTokenID;							// TokenIDǰ׺
	UINT			m_nTokenID_StartNum;					// TokenID��ʼ��
	UINT			m_nClientNum;								// �ͻ���������������
	TestType	m_nTestType;									// ��������
	TIME_THRESHOLD m_time_threshold;				// 
//==========================//
	CDialog *m_pMainDlg;									// �����ڵ���ָ��
//	CArray<CClientEndPoint*> m_arrayClientEndPoint;				// ���ڹ���
	CCriticalSection m_csAddClient;
	CCriticalSection m_csDeleteClient;
	CCriticalSection	m_csProtectCount;
	CClientEndPoint *m_ClientArr;

	BOOL m_bIsDeleteAllClient;
	volatile int  m_nOnlineNum;	
//	UINT m_nOnlineCount;



};
