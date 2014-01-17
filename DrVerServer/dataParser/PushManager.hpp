/*
 * File         : PushManager.hpp
 * Date         : 2012-06-19
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for handle logic
 */


#ifndef PUSHMANAGER_H
#define PUSHMANAGER_H

#include "DrVerData.hpp"
#include "event.hpp"
#include "MessageMgr.hpp"
#include <string>
#include <map>

class PushHandler;

typedef map<string, PushHandler*> PushHandlerMap;
class PushManager
{
public:
    static PushManager* GetInstance();

protected:
    PushManager();
    virtual ~PushManager();

public:
    bool AddToken(const char *pToken, PushHandler *pPushHandler);
    bool RemoveToken(const char *pToken);
    PushHandler* GetPushHandlerWithToken(const char *pToken);
    bool Handle(LPMESSAGE_DATA pMsgData, char *pBody);

    void SendPushMessage(const char *pTokenid, const char *pBody);
    bool Close(int iFd);
    void SendSuccess(int iFd);
    void SendFail(LPMESSAGE_DATA pMsg);

    int GetOnlineDeviceCout();

private:
    PushHandlerMap  m_mapPushHandler;
    PushHandler*    m_aPushHandler[MAX_CONNECTION_COUNT];

    pthread_rwlock_t m_rwlockTokenMap;
};

extern "C" void OnCloseSocket(int iFd);

#endif // PUSHMANAGER_H
