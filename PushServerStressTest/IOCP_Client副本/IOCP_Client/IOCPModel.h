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
	UINT GetCurrentConnectNum();			// 获取当前的并发连接数
	UINT GetRecvPushMsgNum();				// 获取接收的PushMsg数

protected:
	void AddClient2IOCP(CClientEndPoint* pClient);
	void DeleteClient(CClientEndPoint *pClient);						// 删除某个客户端
	void DeleteAllClient(void);												// 删除所有客户端
	static DWORD WINAPI _CalcTimer(LPVOID lParam);						// 计时线程
	static DWORD WINAPI _Connect2PushSrv(LPVOID lParam);			// 检测队列中是否有对象，若有就连到PushSRV
	static DWORD WINAPI _DisConnectPushSrv(LPVOID lParam);			// 检测队列中是否有对象，若有就断开与PushSRV的连接
	static DWORD WINAPI _StartTestTrhead(LPVOID lParam);				// 启动线程
	static DWORD WINAPI _IOWorkerThread(LPVOID lParam);			// I/O处理线程
	


	int GetNumOfProcessors();	
	// 创建一个新的I/O完成端口
	BOOL CreateNewCompletionPort(DWORD NumberOfConcurrentThreads);
	// 将Connect成功的Socket关联到完成端口
	BOOL AssociateDeviceWithCompletionPort(HANDLE hDevice, ULONG_PTR CompletionKey);

	// 释放资源
	BOOL ReleaseResource();

private:
	//HANDLE	 m_hShutdownEvent;						// 通知线程退出的整体
	HANDLE	 m_hIOCP;										// I/O完成端口的句柄
	HANDLE	 *m_phWorkerThreads;					// I/O 工作者线句柄指针
	int				m_nWorkerThreadNum;					// 生成的工作者线程数

//==========================//
	HANDLE	m_hThreadTimer;
	HANDLE	 m_hThreadConn2PushSrv;				// Connect线程句柄
	HANDLE	 m_hThreadDisConn;
	HANDLE	m_hEventStopTest;							// 让线程退出的事件
	HANDLE	m_hEventSafelyDelete;					// 安全delete Client数据的事件

//======Client相关信息设置========//
	DWORD	m_dwServerIP;								// PushServer的IP
	UINT			m_nServerPort;								// PushServer的端口
	string		m_strPreTokenID;							// TokenID前缀
	UINT			m_nTokenID_StartNum;					// TokenID超始号
	UINT			m_nClientNum;								// 客户端数（并发数）
	TestType	m_nTestType;									// 测试类型
	TIME_THRESHOLD m_time_threshold;				// 
//==========================//
	CDialog *m_pMainDlg;									// 主窗口的类指针
//	CArray<CClientEndPoint*> m_arrayClientEndPoint;				// 用于管理
	CCriticalSection m_csAddClient;
	CCriticalSection m_csDeleteClient;
	CCriticalSection	m_csProtectCount;
	CClientEndPoint *m_ClientArr;

	BOOL m_bIsDeleteAllClient;
	volatile int  m_nOnlineNum;	
//	UINT m_nOnlineCount;



};
