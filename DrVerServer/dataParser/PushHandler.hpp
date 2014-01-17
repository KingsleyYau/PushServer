/*
 * File         : PushHandler.hpp
 * Date         : 2012-06-19
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server data parser
 */

#ifndef PUSHHANDLER_H
#define PUSHHANDLER_H

#include "DrVerData.hpp"
#include "MessageMgr.hpp"

typedef struct _tagPushItem
{
    string chanllenge;
    string tokenId;
    string appId;

    _tagPushItem()
    {
        Reset();
    }

    _tagPushItem(const _tagPushItem &pushItem)
    {
        chanllenge = pushItem.chanllenge;
        tokenId = pushItem.tokenId;
        appId = pushItem.appId;
    }

    void Reset()
    {
        chanllenge = "";
        tokenId = "";
        appId = "";
    }
}PushItem;

class LogicHandler;
class PushHandler
{
public:
    PushHandler();
    virtual ~PushHandler();

public:
    static void DataFormatError(LPMESSAGE_DATA pData);

public:
    bool Parsing(char *pBody, LPMESSAGE_DATA pMsgData);
    bool SetInactive(bool bIsDeregisterTokenId = true);
    void SendSuccess();
    bool RegisterTokenId(const char *pTokenId);
    bool DeregisterTokenId(const char *pTokenId);
    bool SendPushMessage(const char *pPushMsg);
    void SetFd(int iFd);
    int GetFd() const;

private:
    inline void ParsingURLString(char *pUrlString);
    inline bool CharTo16Hex(const char cChar, char &cTrans);

private:
    PushItem        m_pushItem;
    LogicHandler    *m_pLogicHandler;
    pthread_mutex_t m_mutexInactive;
    int             m_iFd;
};

#endif // PUSHHANDLER_H
