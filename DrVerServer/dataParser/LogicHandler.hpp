/*
 * File         : ServerHandler.hpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Server, it handles protocol and business logic
 */

#ifndef LOGICHANDLER_H
#define LOGICHANDLER_H

#include "json/json.h"
#include "MessageMgr.hpp"

class PushHandler;
class LogicHandler
{
public:
    LogicHandler(){;};
    virtual ~LogicHandler(){;};
public:
    virtual void SetPushHandler(PushHandler *pPushHandler) = 0;
    virtual bool HandleProtocolData(const Json::Value &value, LPMESSAGE_DATA pMsgData) = 0;
    virtual void Reset(bool bIsDeregisterTokenId = true) = 0;
    virtual void SendSuccess() = 0;
};

// protocol define
// command
#define GET_CLIENT_CHANLLENGE_CMD   "getclientchanllenge"
#define GET_SERVER_CHANLLENGE_CMD   "getserverchanllenge"
#define REGISTER_CMD                "register"
#define GET_PUSH_MESSAGE_CMD        "getpushmessage"
#define PUSHMSG_CMD                 "pushmsg"

// param
#define CMD_PARAM           "cmd"
#define RET_PARAM           "ret"
// ---------------
#define CHANLLENGE_PARAM    "chanllenge"
#define TOKENID_PARAM       "tokenid"
// ---------------
#define APPID_PARAM         "appid"
#define CHECKCODE_PARAM     "checkcode"
#define MODEL_PARAM         "model"             /// client model: P1010
#define SYSTEM_PARAM        "system"            /// system name: android 2.2
#define APPVER_PARAM        "appver"            /// application version: 1.6.1
// ---------------
#define APPLASTVER_PARAM    "applastver"        /// application last version: 1.8.0
#define APPPKGURL_PARAM     "apppkgurl"         /// application last version package url: http://drpalm.doctorcom.com/xxx.apk
#define APPVERDESC_PARAM    "appverdesc"        /// application list version desc: fixed some bugs
// ---------------
#define TOKENIDLIST_PARAM   "tokenidlist"
#define BODY_PARAM          "body"

// push message param
#define CONTEXT_LENGTH_PARAM "Context-Length:%d\r\n\r\n%s"

// http post protocol
#define BODY_END_SIGN       "\r\n\r\n"
#define JSONPARAM_SGIN      "jsonparam="

// error code
#define SUCCESS_CODE            "0"
#define REJECT_CODE             "1001"
#define FORMATERR_CODE          "1101"
#define CHECKCODEERR_CODE       "2001"
#define EXCESSLIMITLENGTH_CODE  "2101"

#endif // LOGICHANDLER_H
