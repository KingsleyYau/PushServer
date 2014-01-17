/*
 * File         : ClientHandler.cpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for PushServer<->Client, it handles protocol and business logic
 */

#include "ClientHandler.hpp"
#include "md5.h"
#include "event.hpp"
#include "PushHandler.hpp"
#include "TimeProc.hpp"
#include "ClientManager.hpp"
#include "SocketProc.hpp"

extern MsgListManager* g_MsgListSnd;

ClientHandler::ClientHandler()
{
    //ctor
    pthread_mutex_init(&m_mutexSendPushMsg, NULL);
    pthread_mutex_init(&m_mutexReset, NULL);
    m_pPushHandler = NULL;
    Reset();
}

ClientHandler::~ClientHandler()
{
    //dtor
    Reset();
    pthread_mutex_destroy(&m_mutexSendPushMsg);
    pthread_mutex_destroy(&m_mutexReset);
}

// LogicHandler interface
void ClientHandler::SetPushHandler(PushHandler *pPushHandler)
{
    m_pPushHandler = pPushHandler;
}

bool ClientHandler::HandleProtocolData(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    CAutoLock lock(&m_mutexReset);
    gvLog(LOG_MSG, "(ClientHandler::HandleProtocolData) MSG: begin m_eHandleStep:%d iFd:%d", m_eHandleStep, pMsgData->iFd);
    {
        Json::FastWriter writer;
        string strJson = writer.write(value);
        gvLog(LOG_MSG, "(ClientHandler::HandleProtocolData) MSG: %s\r\n", strJson.c_str());
    }

    bool bResult = false;
    if (value.isMember(CMD_PARAM) && value[CMD_PARAM].isString())
    {
        string strCmd = value[CMD_PARAM].asString();
        if (0 == strCmd.compare(GET_CLIENT_CHANLLENGE_CMD)
            && GET_CHANLLENGE_STEP == m_eHandleStep)
        {
            bResult = GetChanllengeHandle(value, pMsgData);
        }
        else if (0 == strCmd.compare(REGISTER_CMD)
                 && REGISTER_TOKENID_STEP == m_eHandleStep)
        {
            bResult = RegisterHandle(value, pMsgData);
        }
        else if (0 == strCmd.compare(GET_PUSH_MESSAGE_CMD)
                 && GET_PUSHMESSAGE_STEP == m_eHandleStep)
        {
            bResult = GetPushMessageHandle(value, pMsgData);
        }
    }

    if (!bResult){
        Json::FastWriter writer;
        string strJson = writer.write(value);
        gvLog(LOG_ERR_USER, "(ClientHandler::HandleProtocolData) ERR: protocol error tm_eHandleStep:%d\r\n%s\r\n", m_eHandleStep, strJson.c_str());
    }

    gvLog(LOG_MSG, "(ClientHandler::HandleProtocolData) MSG: end pMsgData->iFd:%d", pMsgData->iFd);
    return bResult;
}

void ClientHandler::Reset(bool bIsDeregisterTokenId)
{
    CAutoLock lockReset(&m_mutexReset);

    if (m_eHandleStep == BEGIN_STEP) {
        // 已经重置
        return ;
    }

    gvLog(LOG_MSG, "(ClientHandler::Reset) MSG: m_strTokenId:%s", m_strTokenId.c_str());

    if (NULL != m_pPushHandler && m_strTokenId.length() > 0 && bIsDeregisterTokenId){
        if (m_pPushHandler->DeregisterTokenId(m_strTokenId.c_str()))
        {
            // 记录在线信息
            ClientManager::GetInstance()->AddOnlineRecord(m_strTokenId.c_str()
                                                          , m_strClientIp.c_str()
                                                          , m_iClientPort
                                                          , m_tStartOnlineTime
                                                          , time(0)
                                                          , m_iSendOfflineMsgCount
                                                          , m_iSendOnlineMsgCount);
        }
    }

    pthread_mutex_lock(&m_mutexSendPushMsg);
    gvLog(LOG_MSG, "(ClientHandler::Reset) MSG: in lock, save msg to db m_strTokenId:%s", m_strTokenId.c_str());
    DBHandler *pDBHandler = DBHandler::GetInstance();
    for (SendPushMessageList::iterator iter = m_listSendPushMessage.begin();
         m_listSendPushMessage.end() != iter;
         iter++)
    {
        if (!iter->bIsInDB){
            pDBHandler->InsertPushMessage(m_strTokenId.c_str(), iter->strMessage.c_str());
            gvLog(LOG_MSG, "(ClientHandler::Reset) MSG: pDBHandler->InsertPushMessage() pTokenid:%s\n pBody:%s", m_strTokenId.c_str(), iter->strMessage.c_str());
        }
    }
    m_listSendPushMessage.clear();
    gvLog(LOG_MSG, "(ClientHandler::Reset) MSG: out lock, m_strTokenId:%s", m_strTokenId.c_str());
    pthread_mutex_unlock(&m_mutexSendPushMsg);

    m_eHandleStep = BEGIN_STEP;
    m_strChanllenge = "";
    m_strTokenId = "";
    m_bIsSendingMsg = false;

    // 清空统计变量
    m_tStartOnlineTime = 0;
    m_iSendOnlineMsgCount = 0;
    m_iSendOfflineMsgCount = 0;
    m_strClientIp = "";
    m_iClientPort = 0;

    gvLog(LOG_MSG, "(ClientHandler::Reset) MSG: end");
}

void ClientHandler::SendSuccess()
{
    if (END_STEP == m_eHandleStep){
        gvLog(LOG_MSG, "(ClientHandler::SendSuccess) MSG: begin");
        SendNextPushMessage();
        gvLog(LOG_MSG, "(ClientHandler::SendSuccess) MSG: end");
    }
}

// the json can handle
bool ClientHandler::CanHandle(const Json::Value &value, const char *pBody)
{
    if (value.isMember(CMD_PARAM) && value[CMD_PARAM].isString()){
        string strCmd = value[CMD_PARAM].asString();
        return 0 == strCmd.compare(GET_CLIENT_CHANLLENGE_CMD);
    }
    return false;
}

bool ClientHandler::SendPushMessage(const char *pBody)
{
    bool result = false;
    CAutoLock lockReset(&m_mutexReset);
    gvLog(LOG_MSG, "(ClientHandler::SendPushMessage) MSG: begin m_mutexReset begin tokenid:%s", m_strTokenId.c_str());

    CAutoLock lockSendPushMsg(&m_mutexSendPushMsg);
    gvLog(LOG_MSG, "(ClientHandler::SendPushMessage) MSG: m_mutexSendPushMsg begin pTokenid:%s", m_strTokenId.c_str());
    gvLog(LOG_MSG, "(ClientHandler::SendPushMessage) MSG: begin iFd:%d m_pPushHandler:%p", m_pPushHandler->GetFd(), m_pPushHandler);
    if (m_strTokenId.length() > 0){
        TSendPushMessageItem item;
        item.bIsInDB = false;
        item.strMessage = pBody;
        m_listSendPushMessage.push_back(item);
        gvLog(LOG_MSG, "(ClientHandler::SendPushMessage) MSG: m_listSendPushMessage.push_back() tokenid:%s m_eHandleStep:%d m_bIsSendingMsg:%d pBody:\r\n%s\r\n", m_strTokenId.c_str(), m_eHandleStep, m_bIsSendingMsg, pBody);

        if (END_STEP == m_eHandleStep && !m_bIsSendingMsg){
            SendNextPushMessage(false);
        }
        result = true;
    }

    gvLog(LOG_MSG, "(ClientHandler::SendPushMessage) MSG: end");
    return result;
}

// handle protocol
bool ClientHandler::GetChanllengeHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    // create chanllenge
    time_t tChanllenge = time(NULL);
    char *pTime = pMsgData->pData->m_cBuff256;
    snprintf(pTime, sizeof(pMsgData->pData->m_cBuff256), "%x%x00%d", GetTickCount(), tChanllenge << 2, pMsgData->iFd);
    m_strChanllenge = pTime;

    // create json
    Json::Value root;
    root[RET_PARAM] = SUCCESS_CODE;
    root[CHANLLENGE_PARAM] = m_strChanllenge.c_str();

    if (value.isMember("TokenID") && value["TokenID"].isString()) {
        root["TokenID"] = value["TokenID"].asString();
    }

    // set send data
    SetSendData(pMsgData, root, HTTP_REPONSE, HTTP_200_OK);

    m_eHandleStep = REGISTER_TOKENID_STEP;

    gvLog(LOG_MSG, "(ClientHandler::GetChanllengeHandle) iFd:%d m_pPushHandler:%p", pMsgData->iFd, m_pPushHandler);

    return true;
}

bool ClientHandler::RegisterHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
   if (value.isMember(TOKENID_PARAM) && value[TOKENID_PARAM].isString()
        && value.isMember(BODY_PARAM) && value[BODY_PARAM].isObject())
   {
        string tokenid = value[TOKENID_PARAM].asString();
        Json::Value valueBody = value[BODY_PARAM];
        if (!tokenid.empty()
            && valueBody.isMember(APPID_PARAM) && valueBody[APPID_PARAM].isString()
            && valueBody.isMember(CHECKCODE_PARAM) && valueBody[CHECKCODE_PARAM].isString())
        {
            // get appkey with appid
            DBHandler *pDBHandler = DBHandler::GetInstance();
            TPushIdentItem item = pDBHandler->GetAppInfoWithAppid(valueBody[APPID_PARAM].asString().c_str());

            // create checkcode
            string strToMD5 = tokenid + m_strChanllenge + item.appkey;
            char *aCheckcode = (char*)pMsgData->pData->m_cBuff256;
            GetMD5String(strToMD5.c_str(), aCheckcode);

            // compare checkcode
            Json::Value root;
            const char *pHttpHead = NULL;
            if(0 == strcmp(aCheckcode, valueBody[CHECKCODE_PARAM].asString().c_str())) {
                if (NULL != m_pPushHandler){
                    if (!m_pPushHandler->RegisterTokenId(tokenid.c_str())) {
                        gvLog(LOG_ERR_USER, "(ClientHandler::RegisterHandle) ERR: pToken:%s iFd:%d m_pPushHandler:%p", m_strTokenId.c_str(), pMsgData->iFd, m_pPushHandler);
                    }
                }

                Json::Value body;
                // update packet
                CheckUpdateVer(tokenid.c_str(), item, valueBody, body);

                if (!body.isNull()) {
                    root[BODY_PARAM] = body;
                }
                root[RET_PARAM] = SUCCESS_CODE;
                pHttpHead = HTTP_REPONSE;

                // 更新客户端信息
                ClientManager::GetInstance()->UpdateClientInfo(tokenid.c_str()
                                                               , item.appid.c_str()
                                                               , valueBody[APPVER_PARAM].asString().c_str()
                                                               , valueBody[MODEL_PARAM].asString().c_str()
                                                               , valueBody[SYSTEM_PARAM].asString().c_str());

                // 记录在线起始时间
                m_tStartOnlineTime = time(0);
                GetRemoteIpAndPort(pMsgData->iFd, m_strClientIp, m_iClientPort);

                // 注册成功
                m_strTokenId = tokenid;
                m_eHandleStep = GET_PUSHMESSAGE_STEP;

                gvLog(LOG_MSG, "(ClientHandler::RegisterHandle) MSG: register ok! pToken:%s iFd:%d m_pPushHandler:%p", m_strTokenId.c_str(), pMsgData->iFd, m_pPushHandler);
            }
            else {
                // 注册失败
                root[RET_PARAM] = CHECKCODEERR_CODE;
                pMsgData->bSendAndClose = true;
                pHttpHead = HTTP_REPONSE_CLOSE;
                m_eHandleStep = BEGIN_STEP;

                gvLog(LOG_MSG, "(ClientHandler::RegisterHandle) MSG: register fail! iFd:%d token:%s chanllenge:%s appkey:%s checkcode:%s\ncheckcode:%s appid:%s m_pPushHandler:%p"
                      , pMsgData->iFd, tokenid.c_str(), m_strChanllenge.c_str(), item.appkey.c_str(), aCheckcode
                      , valueBody[CHECKCODE_PARAM].asString().c_str(), valueBody[APPID_PARAM].asString().c_str(), m_pPushHandler);
            }

            // set send data
            SetSendData(pMsgData, root, pHttpHead, HTTP_200_OK);
            return true;
        }
        gvLog(LOG_ERR_USER, "(ClientHandler::RegisterHandle) ERR: parsing fail! iFd:%d tokenid:%s", pMsgData->iFd, tokenid.c_str());
    }

    // protocol format error
    m_eHandleStep = BEGIN_STEP;
    gvLog(LOG_ERR_USER, "(ClientHandler::RegisterHandle) ERR: parsing fail! iFd:%d", pMsgData->iFd);
    return false;
}

void ClientHandler::CheckUpdateVer(const char *tokenid, const TPushIdentItem &item, const Json::Value &valueBody, Json::Value &body)
{
    if (UPDATESTATUS_OPEN == item.updatestatus
        || (UPDATESTATUS_TEST == item.updatestatus && NULL != strstr(item.testtokenid.c_str(), tokenid)))
    {
        if (valueBody[APPVER_PARAM].isString()
            && !item.applastver.empty() && !item.apppkgurl.empty()
            && IsOldVersion(valueBody[APPVER_PARAM].asString().c_str(), item.applastver.c_str()))
        {
            body[APPLASTVER_PARAM] = item.applastver;
            body[APPPKGURL_PARAM] = item.apppkgurl;
            body[APPVERDESC_PARAM] = item.appverdesc;
        }
    }
}

bool ClientHandler::GetPushMessageHandle(const Json::Value &value, LPMESSAGE_DATA pMsgData)
{
    CAutoLock lock(&m_mutexSendPushMsg);
    gvLog(LOG_MSG, "(ClientHandler::GetPushMessageHandle) MSG: begin pToken:%s iFd:%d", m_strTokenId.c_str(), pMsgData->iFd);

    DBHandler *pDBHandler = DBHandler::GetInstance();
    PushMessageDBList listPushMessage = pDBHandler->GetPushMessageWithTokenId(m_strTokenId.c_str());
    gvLog(LOG_MSG, "(ClientHandler::GetPushMessageHandle) MSG: pToken:%s iFd:%d listPushMessage.size():%d m_pPushHandler:%p", m_strTokenId.c_str(), pMsgData->iFd, listPushMessage.size(), m_pPushHandler);

    TSendPushMessageItem item;
    string strPush = "";
    for (PushMessageDBList::iterator iter = listPushMessage.begin();
         listPushMessage.end() != iter;
         iter++)
    {
        item.bIsInDB = true;
        item.nId = iter->id;
        item.strMessage = iter->message;
        m_listSendPushMessage.push_back(item);
    }
    m_eHandleStep = END_STEP;

    snprintf(pMsgData->pData->m_cRsp, sizeof(pMsgData->pData->m_cRsp), HTTP_REPONSE_NOLENGTH, HTTP_200_OK, HTTP_TEXT);
    pMsgData->pData->m_usRspTotal = strlen(pMsgData->pData->m_cRsp);
    gvLog(LOG_MSG, "(ClientHandler::GetPushMessageHandle) MSG: pMsgData->pData->m_cRsp:\n%s", pMsgData->pData->m_cRsp);

    gvLog(LOG_MSG, "(ClientHandler::GetPushMessageHandle) MSG: end pToken:%s iFd:%d", m_strTokenId.c_str(), pMsgData->iFd);
    return true;
}

void ClientHandler::SendNextPushMessage(bool bIsLock)
{
    if (bIsLock) {
        pthread_mutex_lock(&m_mutexSendPushMsg);
        gvLog(LOG_MSG, "(ClientHandler::SendNextPushMessage) MSG: lock begin");
    }

    SendNextPushMessageProc();

    if (bIsLock) {
        gvLog(LOG_MSG, "(ClientHandler::SendNextPushMessage) MSG: lock end");
        pthread_mutex_unlock(&m_mutexSendPushMsg);
    }
}

void ClientHandler::SendNextPushMessageProc()
{
    gvLog(LOG_MSG, "(ClientHandler::SendNextPushMessage) MSG: begin tokenid:%s iFd:%d m_pPushHandler:%p", m_strTokenId.c_str(), m_pPushHandler->GetFd(), m_pPushHandler);

    // remove last message
    /// 发送下一条时才删除上一条，是为了防止重复操作数据库
    /// 如果加入发送队列马上删除记录，当发送失败又要把记录插入到数据库
    if (m_bIsSendingMsg){
        SendPushMessageList::iterator iter = m_listSendPushMessage.begin();
        if (m_listSendPushMessage.end() != iter){
            if (iter->bIsInDB){
                // remove with db
                DBHandler::GetInstance()->RemovePushMessage(iter->nId);
                // 已发出的离线消息数统计
                m_iSendOfflineMsgCount++;
            }
            else {
                // 已发出的在线消息数统计
                m_iSendOnlineMsgCount++;
            }
            gvLog(LOG_MSG, "(ClientHandler::SendNextPushMessage) WAR: remove push tokenid:%s message:%s iFd:%d m_pPushHandler:%p", m_strTokenId.c_str(), iter->strMessage.c_str(), m_pPushHandler->GetFd(), m_pPushHandler);
            m_listSendPushMessage.pop_front();
        }
        else {
            gvLog(LOG_WARNING, "(ClientHandler::SendNextPushMessage) WAR: tokenid:%s m_listSendPushMessage is empty, m_bIsSendingMsg=true", m_strTokenId.c_str());
        }
    }

    // get message
    SendPushMessageList::iterator iter = m_listSendPushMessage.begin();
    if (m_listSendPushMessage.end() != iter){
        m_bIsSendingMsg = true;

        // message to json
        Json::Value jsonBody;
        Json::Features features;
        Json::Reader reader(features);
        reader.parse(iter->strMessage, jsonBody);

        // create json
        Json::Value root;
        root[CMD_PARAM] = PUSHMSG_CMD;
        root[TOKENID_PARAM] = m_strTokenId;
        root[BODY_PARAM] = jsonBody;

        // json to string
        string strPushMsg;
        Json::FastWriter writer;
        strPushMsg = writer.write(root);

        LPMESSAGE_DATA pMsgData = GetIdleMsgBuff();
        if (NULL != pMsgData && NULL != m_pPushHandler){
            pMsgData->reset();

            // set msgdata
            snprintf(pMsgData->pData->m_cRsp, sizeof(pMsgData->pData->m_cRsp), CONTEXT_LENGTH_PARAM, strPushMsg.length(), strPushMsg.c_str());
            pMsgData->pData->m_usRspTotal = strlen(pMsgData->pData->m_cRsp);
            pMsgData->iFd = m_pPushHandler->GetFd();
            strcpy(pMsgData->cBuffer, m_strTokenId.c_str());
            pMsgData->sTimes = 3;
            pMsgData->bDBData = iter->bIsInDB;

            gvLog(LOG_MSG, "(ClientHandler::SendNextPushMessage) MSG: will send tokenid:%s bIsInDB:%d iFd:%d \nmessage:%s\n"
                  , m_strTokenId.c_str(), iter->bIsInDB, pMsgData->iFd, pMsgData->pData->m_cRsp);

            g_MsgListSnd->Put_Msg(pMsgData);
        }
        else {
            gvLog(LOG_ERR_SYS, "(ClientHandler::SendNextPushMessage) ERR: pMsgData:%p m_pPushHandler:%p iFd:%d", pMsgData, m_pPushHandler, m_pPushHandler->GetFd());
        }
    }
    else {
        gvLog(LOG_WARNING, "(ClientHandler::SendNextPushMessage) WAR: tokenid:%s iFd:%d m_pPushHandler:%p m_listSendPushMessage is empty!", m_strTokenId.c_str(), m_pPushHandler->GetFd(), m_pPushHandler);
        m_bIsSendingMsg = false;
    }
}

void ClientHandler::SetSendData(LPMESSAGE_DATA pMsgData, const Json::Value &value, const char *pHttpHeader, const char *pHttpStatus)
{
    Json::FastWriter writer;
    string strJson = writer.write(value);
    snprintf(pMsgData->pData->m_cRsp, sizeof(pMsgData->pData->m_cRsp), pHttpHeader, pHttpStatus, HTTP_TEXT, strJson.length());
    strcat(pMsgData->pData->m_cRsp, strJson.c_str());
    pMsgData->pData->m_usRspTotal = strlen(pMsgData->pData->m_cRsp);

    gvLog(LOG_MSG, "(ClientHandler::SetSendData) MSG: m_pPushHandler:%p iFd:%d pMsgData->pData->m_cRsp:\r\n%s\r\n", m_pPushHandler, m_pPushHandler->GetFd(), pMsgData->pData->m_cRsp);
}

string ClientHandler::GetBodyWithMsg(const char *msg)
{
    string strBody;
    const char *strJson = strstr(msg, BODY_END_SIGN);
    strJson = (NULL == strJson ? strJson : strJson + strlen(BODY_END_SIGN));

    Json::Value jsonMsg;
    Json::Features features;
    Json::Reader reader(features);
    if (NULL != strJson && reader.parse(strJson, jsonMsg))
    {
        if (jsonMsg.isObject())
        {
            if (jsonMsg[BODY_PARAM].isObject())
            {
                Json::FastWriter writer;
                strBody = writer.write(jsonMsg[BODY_PARAM]);
            }
        }
    }

    return strBody;
}

bool ClientHandler::IsOldVersion(const char* clientVersion, const char* newVersion)
{
//    gvLog(LOG_MSG, "(ClientHandler::IsOldVersion) MSG: clientVersion:%s newVersion:%s", clientVersion, newVersion);

    if (NULL == newVersion || *newVersion == '\0')
    {
        return false;
    }
    else if (NULL == clientVersion || *clientVersion == '\0')
    {
        return true;
    }

    string strClientVersion = clientVersion;
    string strNewVersion = newVersion;
    bool bResult = strcmp(newVersion, clientVersion) > 0;
    typedef list<int> TVersionList;
    const char* delimit = ".";

    TVersionList clientVerList;
    char* clientVerTemp = strtok((char*)strClientVersion.c_str(), delimit);
    while (NULL != clientVerTemp) {
        clientVerList.push_back(atoi(clientVerTemp));
//        gvLog(LOG_MSG, "(ClientHandler::IsOldVersion) MSG: clientVerList:%s", clientVerTemp);
        clientVerTemp = strtok(NULL, delimit);
    }

    TVersionList newVerList;
    const char* newVerTemp = strtok((char*)strNewVersion.c_str(), delimit);
    while (NULL != newVerTemp) {
        newVerList.push_back(atoi(newVerTemp));
//        gvLog(LOG_MSG, "(ClientHandler::IsOldVersion) MSG: newVerList:%s", newVerTemp);
        newVerTemp = strtok(NULL, delimit);
    }

    int iClientSubVer;
    int iNewSubVer;
    if (clientVerList.size() > 0 && newVerList.size() > 0) {
        bResult = false;
        do{
            iClientSubVer = clientVerList.front();
            clientVerList.pop_front();
            iNewSubVer = newVerList.front();
            newVerList.pop_front();

//            gvLog(LOG_MSG, "(ClientHandler::IsOldVersion) MSG: iClientSubVer:%d iNewSubVer:%d", iClientSubVer, iNewSubVer);

            if (iNewSubVer > iClientSubVer) {
                bResult = true;
                break;
            }
            else if (iNewSubVer < iClientSubVer) {
                break;
            }
            else if (clientVerList.size() == 0 && newVerList.size() != 0) {
                bResult = true;
                break;
            }
        } while (clientVerList.size() > 0 && newVerList.size() > 0);
    }

    return bResult;
}
