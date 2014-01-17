/*
 * File         : ClientHandler.hpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Client, it handles protocol and business logic
 */

#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "DrVerData.hpp"
#include "DBHandler.hpp"
#include "LogicHandler.hpp"
#include <list>

typedef struct _tagSendPushMessageItem
{
    string  strMessage;
    bool    bIsInDB;
    int     nId;
    _tagSendPushMessageItem(){}
    _tagSendPushMessageItem(const _tagSendPushMessageItem &item)
    {
        strMessage = item.strMessage;
        bIsInDB = item.bIsInDB;
        nId = item.nId;
    }
}TSendPushMessageItem;

typedef list<TSendPushMessageItem> SendPushMessageList;

class PushHandler;
class ClientHandler : public LogicHandler
{
private:
    typedef enum {
        BEGIN_STEP = 0,
        GET_CHANLLENGE_STEP = BEGIN_STEP,
        REGISTER_TOKENID_STEP,
        GET_PUSHMESSAGE_STEP,
        END_STEP,
    }TClientHandlerStep;

public:
    ClientHandler();
    virtual ~ClientHandler();

// LogicHandler interface
public:
    virtual void SetPushHandler(PushHandler *pPushHandler);
    virtual bool HandleProtocolData(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    virtual void Reset(bool bIsDeregisterTokenId = true);
    virtual void SendSuccess();

public:
    static bool CanHandle(const Json::Value &value, const char *pBody);
    static string GetBodyWithMsg(const char *msg);
    bool SendPushMessage(const char *pBody);

private:
    // handle protocol
    inline bool GetChanllengeHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    inline bool RegisterHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    inline void CheckUpdateVer(const char *tokenid, const TPushIdentItem &item, const Json::Value &valueBody, Json::Value &body);
    inline bool GetPushMessageHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    inline void SendNextPushMessage(bool bIsLock = true);
    inline void SendNextPushMessageProc();
    inline void SetSendData(LPMESSAGE_DATA pMsgData, const Json::Value &value, const char *pHttpHeader, const char *pHttpStatus);
    inline bool IsOldVersion(const char* clientVersion, const char* newVersion);

private:
    PushHandler *m_pPushHandler;
    string  m_strTokenId;
    string  m_strChanllenge;
    TClientHandlerStep  m_eHandleStep;
    SendPushMessageList m_listSendPushMessage;
    pthread_mutex_t m_mutexSendPushMsg;
    pthread_mutex_t m_mutexReset;
    bool    m_bIsSendingMsg;

    // 用于统计
    time_t  m_tStartOnlineTime;
    int     m_iSendOnlineMsgCount;
    int     m_iSendOfflineMsgCount;
    string  m_strClientIp;
    int     m_iClientPort;
};

#endif // CLIENTHANDLER_H
