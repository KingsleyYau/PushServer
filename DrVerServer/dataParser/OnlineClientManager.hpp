/*
 * File         : OnlineClientManager.hpp
 * Date         : 2012-06-21
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for online client manager
 */

#ifndef ONLINECLIENTMANAGER_H
#define ONLINECLIENTMANAGER_H

#include <map>
#include <string>
#include "DrVerData.hpp"
#include "event.hpp"

typedef struct _tagOnlineClientItem
{
    int socket;
}TOnlineClientItem;

typedef map < string, TOnlineClientItem, less< string > > OnlineClientMap;

class OnlineClientManager
{
public:
    static OnlineClientManager *GetInstance();

protected:
    OnlineClientManager();
    virtual ~OnlineClientManager();

public:
    bool GetOnlineClientWithTokenId(const char *pTokenId, TOnlineClientItem &item);
    bool InsertOnlineClient(const char *pTokenId, const TOnlineClientItem &item);
    bool RemoveOnlineClient(const char *pTokenId);

private:
    OnlineClientMap m_mapOnlineClient;
    pthread_mutex_t m_mutex;
};

#endif // ONLINECLIENTMANAGER_H
