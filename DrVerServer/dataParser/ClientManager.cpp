/*
 * File         : ClientManager.cpp
 * Date         : 2012-11-14
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Server, it handles protocol and business logic
 */

#include "ClientManager.hpp"
#include "DBHandler.hpp"
#include "event.hpp"

#define TransSpareIndex(spare)  (spare ? 1 : 0)

ClientManager* ClientManager::GetInstance()
{
    static ClientManager g_clientManager;
    return &g_clientManager;
}

ClientManager::ClientManager()
{
    pthread_mutex_init(&m_mutexClientInfo, NULL);
    pthread_mutex_init(&m_mutexStorageClientInfo, NULL);
    pthread_mutex_init(&m_mutexOnlineRecord, NULL);
    pthread_mutex_init(&m_mutexStorageOnlineRecord, NULL);
    m_isSpareClientInfoMap = false;
    m_isSpareOnlineRecordList = false;
}

ClientManager::~ClientManager()
{
    pthread_mutex_destroy(&m_mutexClientInfo);
    pthread_mutex_destroy(&m_mutexStorageClientInfo);
    pthread_mutex_destroy(&m_mutexOnlineRecord);
    pthread_mutex_destroy(&m_mutexStorageOnlineRecord);
}

/// ----------------- 客户端信息 -----------------
// 更新客户端信息
bool ClientManager::UpdateClientInfo(const char* tokenid
                                     , const char*appid
                                     , const char* appver
                                     , const char* model
                                     , const char* system)
{
    if (NULL == tokenid || 0 == *tokenid){
        return false;
    }

    CAutoLock lock(&m_mutexClientInfo);
    int iMapIndex = TransSpareIndex(m_isSpareClientInfoMap);
    TClientInfoMap* mapClientInfo = &m_mapClientInfo[iMapIndex];

    TClientInfoItem item;
    item.tokenid = tokenid;
    item.appid = appid;
    item.appver = appver;
    item.model = model;
    item.system = system;

    pair< TClientInfoMap::iterator, bool > result;
    result = mapClientInfo->insert(TClientInfoMap::value_type(tokenid, item));
    if (!result.second){
        // tokenid exist
        mapClientInfo->erase(tokenid);
        result = mapClientInfo->insert(TClientInfoMap::value_type(tokenid, item));
    }
    return result.second;
}

// 切换客户端信息表（切换后返回原来的表，方便异步写入数据库，使用后需要把该表清空）
TClientInfoMap* ClientManager::ChangeClientInfoMap()
{
    CAutoLock lock(&m_mutexClientInfo);
    int iMapIndex = TransSpareIndex(m_isSpareClientInfoMap);
    m_isSpareClientInfoMap = !m_isSpareClientInfoMap;
    return &m_mapClientInfo[iMapIndex];
}

// 把客户端信息入库
bool ClientManager::StorageClientInfoMap()
{
    CAutoLock lock(&m_mutexStorageClientInfo);
    TClientInfoMap* mapClientInfo = ChangeClientInfoMap();
    for (TClientInfoMap::iterator iter = mapClientInfo->begin();
        mapClientInfo->end() != iter;
        iter++)
    {
        DBHandler::GetInstance()->ReplaceClientInfo((*iter).second);
    }
    mapClientInfo->clear();
    return true;
}

/// ----------------- 客户端在线记录 -----------------
// 添加在线记录
bool ClientManager::AddOnlineRecord(const char* tokenid
                                    , const char* ip
                                    , int port
                                    , time_t start
                                    , time_t end
                                    , int iOfflineMsg
                                    , int iOnlineMsg)
{
    if (NULL == tokenid || 0 == *tokenid
        || NULL == ip || 0 == *ip
        || 0 == port)
    {
        return false;
    }

    CAutoLock lock(&m_mutexOnlineRecord);
    int iListIndex = TransSpareIndex(m_isSpareOnlineRecordList);
    TOnlineRecordList* listOnlineRecord = &m_listOnlineRecord[iListIndex];

    TOnlineRecordItem item;
    item.tokenid = tokenid;
    item.ip = ip;
    item.port = port;
    item.start = start;
    item.end = end;
    item.offlineMsg = iOfflineMsg;
    item.onlineMsg = iOnlineMsg;

    size_t size = listOnlineRecord->size();
    listOnlineRecord->push_back(item);
    return listOnlineRecord->size() > size;
}

// 切换在线记录表（切换后返回原来的表，方便异步写入数据库，使用后需要把该表清空）
TOnlineRecordList* ClientManager::ChangeOnlineRecordList()
{
    CAutoLock lock(&m_mutexOnlineRecord);
    int iListIndex = TransSpareIndex(m_isSpareOnlineRecordList);
    m_isSpareOnlineRecordList = !m_isSpareOnlineRecordList;
    return &m_listOnlineRecord[iListIndex];
}

// 把在线记录表入库
bool ClientManager::StorageOnlineRecordList()
{
    CAutoLock lock(&m_mutexStorageOnlineRecord);
    TOnlineRecordList* listOnlineRecord = ChangeOnlineRecordList();
    for (TOnlineRecordList::iterator iter = listOnlineRecord->begin();
        listOnlineRecord->end() != iter;
        iter++)
    {
        DBHandler::GetInstance()->InsertClientOnlineRecord(*iter);
    }
    listOnlineRecord->clear();
    return true;
}
