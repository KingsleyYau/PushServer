/*
 * File         : MatchPacketHandler.hpp
 * Date         : 2012-06-21
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for match packet handling
 */


#ifndef MATCHPACKETHANDLER_H
#define MATCHPACKETHANDLER_H

#include "DrVerData.hpp"
#include "MessageMgr.hpp"

#define IsAvailableIndex(index) (index >= 0 && index < MAX_CONNECTION_COUNT)

class MatchPacketHandler
{
public:
    static MatchPacketHandler *GetInstance();

protected:
    MatchPacketHandler();
    virtual ~MatchPacketHandler();

public:
    LPMESSAGE_DATA Handling(LPMESSAGE_DATA pMsgData, LPMESSAGE_DATA &pErrMsgData);

private:
    inline bool GetContextLength(LPMESSAGE_DATA pMsgData, unsigned int &nHeaderLength, unsigned int &nContextLength);

private:
    LPMESSAGE_DATA  m_aMsgData[MAX_CONNECTION_COUNT];
};

#endif // MATCHPACKETHANDLER_H
