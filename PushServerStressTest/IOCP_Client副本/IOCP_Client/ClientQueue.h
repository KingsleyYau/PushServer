#pragma once
#include <list>
#include <afxmt.h>
#include "ClientEndPoint.h"
using namespace std;

class ClientQueue
{
public:
	ClientQueue(void);
	virtual ~ClientQueue(void);
	BOOL IsEmpty();
	void Clear();
	void Push2Queue(CClientEndPoint* const pClient);
	CClientEndPoint* PopFormQueue(void);
protected:
	list<CClientEndPoint*>	m_ClientQueue;
	CCriticalSection				m_csEnterQueue;
	HANDLE						m_hSemaphore;
};

class ClientConnQueue : public ClientQueue
{
public:
	static ClientConnQueue* GetInstance(void);

};

class ClientDisConnQueue : public ClientQueue
{
public:
	static ClientDisConnQueue* GetInstance(void);

};