/*
 * File         : OnlineClientManager.cpp
 * Date         : 2012-06-21
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for online client manager
 */


#include "OnlineClientManager.hpp"

OnlineClientManager *OnlineClientManager::GetInstance()
{
    static OnlineClientManager onlineClientManager;
    return &onlineClientManager;
}

OnlineClientManager::OnlineClientManager()
{
    //ctor
    pthread_mutex_init(&m_mutex, NULL);
}

OnlineClientManager::~OnlineClientManager()
{
    //dtor
    pthread_mutex_destroy(&m_mutex);
}

bool OnlineClientManager::GetOnlineClientWithTokenId(const char *pTokenId, TOnlineClientItem &item)
{
    CAutoLock lock(&m_mutex);
    OnlineClientMap::iterator iter = m_mapOnlineClient.find(pTokenId);
    if (iter != m_mapOnlineClient.end()){
        item = iter->second;
        return true;
    }
    return false;
}

bool OnlineClientManager::InsertOnlineClient(const char *pTokenId, const TOnlineClientItem &item)
{
    CAutoLock lock(&m_mutex);
    OnlineClientMap::iterator iter = m_mapOnlineClient.find(pTokenId);
    if (iter != m_mapOnlineClient.end()){
        iter->second = item;

    }
    else{
        m_mapOnlineClient.insert( OnlineClientMap::value_type(pTokenId, item) );
    }
    return true;
}

bool OnlineClientManager::RemoveOnlineClient(const char *pTokenId)
{
    CAutoLock lock(&m_mutex);
    OnlineClientMap::iterator iter = m_mapOnlineClient.find(pTokenId);
    if (iter != m_mapOnlineClient.end()){
        m_mapOnlineClient.erase(iter);
        return true;
    }
    return false;
}
