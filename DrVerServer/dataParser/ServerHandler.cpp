/*
 * File         : ServerHandler.cpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Server, it handles protocol and business logic
 */

#include "ServerHandler.hpp"
#include "PushHandler.hpp"
#include "PushManager.hpp"
#include "DBHandler.hpp"
#include "md5.h"
#include "TimeProc.hpp"

ServerHandler::ServerHandler()
{
    //ctor
    m_pPushHandler = NULL;
    pthread_mutex_init(&m_mutexReset, NULL);
    Reset();
}

ServerHandler::~ServerHandler()
{
    //dtor
    pthread_mutex_destroy(&m_mutexReset);
}

// LogicHandler interface
void ServerHandler::SetPushHandler(PushHandler *pPushHandler)
{
    m_pPushHandler = pPushHandler;
}

bool ServerHandler::HandleProtocolData(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    CAutoLock lock(&m_mutexReset);
    bool result = false;
    gvLog(LOG_MSG, "(ServerHandler::HandleProtocolData) MSG: in lock");
    if (value.isMember(CMD_PARAM) && value[CMD_PARAM].isString()){
        string strCmd = value[CMD_PARAM].asString();
        if (0 == strCmd.compare(GET_SERVER_CHANLLENGE_CMD)
            && GET_CHANLLENGE_STEP == m_eHandleStep)
        {
            result = GetChanllengeHandle(value, pMsgData);
        }
        else if (0 == strCmd.compare(REGISTER_CMD)
                 && REGISTER_SERVER_STEP == m_eHandleStep)
        {
            result = RegisterHandle(value, pMsgData);
        }
        else if (0 == strCmd.compare(PUSHMSG_CMD)
                 && PUSHMESSAGE_STEP == m_eHandleStep)
        {
            Json::FastWriter writer;
            string strJson = writer.write(value);
            gvLog(LOG_MSG, "(ServerHandler::HandleProtocolData) MSG: PushMessageHandle body:%s", strJson.c_str());
            result = PushMessageHandle(value, pMsgData);
        }
        else {
            gvLog(LOG_ERR_USER, "(ServerHandler::HandleProtocolData) ERR: strCmd:%s\tm_eHandleStep:%d", strCmd.c_str(), m_eHandleStep);
        }
    }
    else {
        gvLog(LOG_ERR_USER, "(ServerHandler::HandleProtocolData) ERR: CMD_PARAM not found");
    }

    gvLog(LOG_MSG, "(ServerHandler::HandleProtocolData) MSG: out lock");
    return result;
}

void ServerHandler::Reset(bool bIsDeregisterTokenId)
{
    CAutoLock lock(&m_mutexReset);
    gvLog(LOG_MSG, "(ServerHandler::Reset) MSG: in lock");
    m_eHandleStep = BEGIN_STEP;
    m_strChanllenge = "";
    gvLog(LOG_MSG, "(ServerHandler::Reset) MSG: out lock");
}

void ServerHandler::SendSuccess()
{

}

// the json can handle
bool ServerHandler::CanHandle(const Json::Value &value, const char *pBody)
{
    if (value.isMember(CMD_PARAM) && value[CMD_PARAM].isString()){
        string strCmd = value[CMD_PARAM].asString();
        return 0 == strCmd.compare(GET_SERVER_CHANLLENGE_CMD);
    }
    return false;
}

// handle protocol
bool ServerHandler::GetChanllengeHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    gvLog(LOG_MSG, "(ServerHandler::GetChanllengeHandle) MSG: begin iFd:%d", pMsgData->iFd);

    // create chanllenge
    time_t tChanllenge = time(NULL);
    char *pTime = pMsgData->pData->m_cBuff256;
    snprintf(pTime, sizeof(pMsgData->pData->m_cBuff256), "%x%x00%d", GetTickCount(), tChanllenge << 2, pMsgData->iFd);
    m_strChanllenge = pTime;

    // create json
    Json::Value root;
    root[RET_PARAM] = SUCCESS_CODE;
    root[CHANLLENGE_PARAM] = m_strChanllenge.c_str();

    // set send data
    SetSendData(pMsgData, root, HTTP_REPONSE, HTTP_200_OK);

    m_eHandleStep = REGISTER_SERVER_STEP;
    return true;
}

bool ServerHandler::RegisterHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    if (value.isMember(BODY_PARAM) && value[BODY_PARAM].isObject())
   {
        Json::Value valueBody = value[BODY_PARAM];
        if (valueBody.isMember(APPID_PARAM) && valueBody[APPID_PARAM].isString() && !valueBody[APPID_PARAM].asString().empty()
            && valueBody.isMember(CHECKCODE_PARAM) && valueBody[CHECKCODE_PARAM].isString())
        {
            // get appkey with appid
            DBHandler *pDBHandler = DBHandler::GetInstance();
            TPushIdentItem item = pDBHandler->GetAppInfoWithAppid(valueBody[APPID_PARAM].asString().c_str());

            // create checkcode
            string strToMD5 = valueBody[TOKENID_PARAM].asString() + m_strChanllenge + item.appkey;
            char *aCheckcode = (char*)pMsgData->pData->m_cBuff256;
            GetMD5String(strToMD5.c_str(), aCheckcode);
            gvLog(LOG_MSG, "(ServerHandler::RegisterHandle) MSG: iFd:%d strToMD5:%s strCheckcode:%s CHECKCODE:%s"
                  , pMsgData->iFd, strToMD5.c_str(), aCheckcode, valueBody[CHECKCODE_PARAM].asString().c_str());

            // compare checkcode
            Json::Value root;
            const char *pHttpHead = NULL;
            if(0 == strcmp(aCheckcode, valueBody[CHECKCODE_PARAM].asString().c_str())) {
                root[RET_PARAM] = SUCCESS_CODE;
                pHttpHead = HTTP_REPONSE;
                m_eHandleStep = PUSHMESSAGE_STEP;
            }
            else {
                root[RET_PARAM] = CHECKCODEERR_CODE;
                pMsgData->bSendAndClose = true;
                pHttpHead = HTTP_REPONSE_CLOSE;
                m_eHandleStep = BEGIN_STEP;
            }
            // set send data
            SetSendData(pMsgData, root, pHttpHead, HTTP_200_OK);

            return true;
        }
    }

    // protocol format error
    m_eHandleStep = BEGIN_STEP;

    gvLog(LOG_MSG, "(ServerHandler::RegisterHandle) MSG: protocol error iFd:%d", pMsgData->iFd);
    return false;
}

bool ServerHandler::PushMessageHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
//    CAutoLock lock(&m_mutexSendPushMsg);
    gvLog(LOG_MSG, "(ServerHandler::PushMessageHandle) MSG: begin iFd:%d", pMsgData->iFd);

    if (value.isMember(BODY_PARAM) && value[BODY_PARAM].isObject())
    {
        Json::Value valueBody = value[BODY_PARAM];
        if (valueBody.isMember(TOKENIDLIST_PARAM) && valueBody[TOKENIDLIST_PARAM].isArray()
            && valueBody.isMember(BODY_PARAM) && valueBody[BODY_PARAM].isObject())
        {
            Json::FastWriter writer;
            string strBody = writer.write(valueBody[BODY_PARAM]);
            gvLog(LOG_MSG, "(ServerHandler::PushMessageHandle) MSG: iFd:%d body:\n%s", pMsgData->iFd, strBody.c_str());
            for (Json::Value::iterator iter = valueBody[TOKENIDLIST_PARAM].begin();
                 valueBody[TOKENIDLIST_PARAM].end() != iter;
                 iter++)
            {
                if ((*iter).isMember(TOKENID_PARAM) && (*iter)[TOKENID_PARAM].isString())
                {
                    std::string tokenid = (*iter)[TOKENID_PARAM].asString();
                    if (!tokenid.empty())
                    {
                        PushManager::GetInstance()->SendPushMessage(tokenid.c_str(), strBody.c_str());
                    }
                }
            }
            Json::Value root;
            root[RET_PARAM] = SUCCESS_CODE;
            SetSendData(pMsgData, root, HTTP_REPONSE, HTTP_200_OK);
            return true;
        }
    }

    gvLog(LOG_ERR_USER, "(ServerHandler::PushMessageHandle) MSG: protocol error iFd:%d", pMsgData->iFd);
    return false;
}

void ServerHandler::SetSendData(LPMESSAGE_DATA pMsgData, const Json::Value &value, const char *pHttpHead, const char *pHttpStatus)
{
    Json::FastWriter writer;
    string strJson = writer.write(value);
    snprintf(pMsgData->pData->m_cRsp, sizeof(pMsgData->pData->m_cRsp), pHttpHead, pHttpStatus, HTTP_TEXT, strJson.length());
    strcat(pMsgData->pData->m_cRsp, strJson.c_str());
    pMsgData->pData->m_usRspTotal = strlen(pMsgData->pData->m_cRsp);

    gvLog(LOG_MSG, "(ServerHandler::SetSendData) MSG: iFd:%d pMsgData->pData->m_cRsp:%s", pMsgData->iFd, pMsgData->pData->m_cRsp);
}
