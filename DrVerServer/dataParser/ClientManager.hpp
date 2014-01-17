/*
 * File         : ClientManager.hpp
 * Date         : 2012-11-14
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : 客户端管理类，管理客户端信息，在线记录等
 */

#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include "DrVerData.hpp"
#include <map>
#include <list>

using namespace std;

// 客户端信息
typedef struct _tagClientInfoItem
{
    string  tokenid;    // 唯一标识
    string  appid;      // 应用id
    string  appver;     // 应用的版本号
    string  model;      // 型号
    string  system;     // 系统版本
    _tagClientInfoItem() {}
    _tagClientInfoItem(const _tagClientInfoItem &item)
    {
        tokenid = item.tokenid;
        appid = item.appid;
        appver = item.appver;
        model = item.model;
        system = item.system;
    }
}TClientInfoItem;
// 客户端信息表
typedef map<string, TClientInfoItem>    TClientInfoMap;

// 在线记录
typedef struct _tagOnlineRecordItem
{
    string  tokenid;    // 唯一标识
    string  ip;         // 连接ip
    int     port;       // 连接的端口
    time_t  start;      // 上线时间
    time_t  end;        // 离线时间
    int     offlineMsg; // 收到的离线消息数
    int     onlineMsg;  // 收到的在线消息数
    _tagOnlineRecordItem(){}
    _tagOnlineRecordItem(const _tagOnlineRecordItem &item)
    {
        tokenid = item.tokenid;
        ip = item.ip;
        port = item.port;
        start = item.start;
        end = item.end;
        offlineMsg = item.offlineMsg;
        onlineMsg = item.onlineMsg;
    }
}TOnlineRecordItem;
// 在线记录表
typedef list<TOnlineRecordItem> TOnlineRecordList;

class ClientManager
{
public:
    static ClientManager* GetInstance();

public:
    ClientManager();
    virtual ~ClientManager();

public:
    // 更新客户端信息
    bool UpdateClientInfo(const char* tokenid, const char*appid, const char* appver, const char* model, const char* system);
    // 把客户端信息入库
    bool StorageClientInfoMap();

    // 添加在线记录
    bool AddOnlineRecord(const char* tokenid, const char* ip, int port, time_t start, time_t end, int iOfflineMsg, int iOnlineMsg);
    // 把在线记录表入库
    bool StorageOnlineRecordList();

private:
    // 切换客户端信息表（切换后返回原来的表，方便异步写入数据库，使用后需要把该表清空）
    TClientInfoMap* ChangeClientInfoMap();
    // 切换在线记录表（切换后返回原来的表，方便异步写入数据库，使用后需要把该表清空）
    TOnlineRecordList* ChangeOnlineRecordList();

private:
    TClientInfoMap      m_mapClientInfo[2];
    bool                m_isSpareClientInfoMap;
    pthread_mutex_t     m_mutexClientInfo;
    pthread_mutex_t     m_mutexStorageClientInfo;

    TOnlineRecordList   m_listOnlineRecord[2];
    bool                m_isSpareOnlineRecordList;
    pthread_mutex_t     m_mutexOnlineRecord;
    pthread_mutex_t     m_mutexStorageOnlineRecord;
};

#endif // CLIENTMANAGER_H
