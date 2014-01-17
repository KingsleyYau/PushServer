#include "StdAfx.h"
#include "ClientQueue.h"

ClientQueue::ClientQueue(void)
{
	m_hSemaphore = ::CreateSemaphore(NULL, 0, 30000, NULL);
}

ClientQueue::~ClientQueue(void)
{
	CloseHandle(m_hSemaphore);
}

BOOL ClientQueue::IsEmpty()
{
	return m_ClientQueue.empty();
}
void ClientQueue::Clear()
{
	while( !m_ClientQueue.empty())
	{
		PopFormQueue();
	}
}
void ClientQueue::Push2Queue(CClientEndPoint* const pClient)
{
	m_csEnterQueue.Lock();
	m_ClientQueue.push_back(pClient);
	ReleaseSemaphore(m_hSemaphore, 1, NULL);
	m_csEnterQueue.Unlock();
}
CClientEndPoint* ClientQueue::PopFormQueue(void)
{
	CClientEndPoint *pClient = NULL;
	WaitForSingleObject(m_hSemaphore, 500);	
	m_csEnterQueue.Lock();
	if (!m_ClientQueue.empty())
	{
		pClient = m_ClientQueue.front();
		m_ClientQueue.pop_front();
	} 
	m_csEnterQueue.Unlock();
	return pClient;
}

ClientConnQueue* ClientConnQueue::GetInstance()
{
	static ClientConnQueue g_ClientConnQueue;
	return &g_ClientConnQueue;
}
ClientDisConnQueue* ClientDisConnQueue::GetInstance()
{
	static ClientDisConnQueue g_ClientDisconnQueue;
	return &g_ClientDisconnQueue;
}
