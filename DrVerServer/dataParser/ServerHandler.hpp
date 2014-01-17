/*
 * File         : ServerHandler.hpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Server, it handles protocol and business logic
 */

#ifndef SERVERHANDLER_H
#define SERVERHANDLER_H

#include "DrVerData.hpp"
#include "LogicHandler.hpp"

class PushHandler;
class ServerHandler : public LogicHandler
{
protected:
    typedef enum {
        BEGIN_STEP = 0,
        GET_CHANLLENGE_STEP = BEGIN_STEP,
        REGISTER_SERVER_STEP,
        PUSHMESSAGE_STEP,
        END_STEP,
    }TServerHandlerStep;

public:
    ServerHandler();
    virtual ~ServerHandler();

// LogicHandler interface
public:
    virtual void SetPushHandler(PushHandler *pPushHandler);
    virtual bool HandleProtocolData(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    virtual void Reset(bool bIsDeregisterTokenId = true);
    virtual void SendSuccess();

private:
    // handle protocol
    inline bool GetChanllengeHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    inline bool RegisterHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);
    inline bool PushMessageHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData);

    inline void SetSendData(LPMESSAGE_DATA pMsgData, const Json::Value &value, const char *pHttpHead, const char *pHttpStatus);

public:
    static bool CanHandle(const Json::Value &value, const char *pBody);

private:
    PushHandler *m_pPushHandler;
    TServerHandlerStep  m_eHandleStep;
    string  m_strChanllenge;
    pthread_mutex_t m_mutexReset;
};

#endif // SERVERHANDLER_H
