/*
 * File         : PushHandler.cpp
 * Date         : 2012-06-19
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for handle logic
 */

#include "PushHandler.hpp"
#include "LogicHandler.hpp"
#include "ClientHandler.hpp"
#include "ServerHandler.hpp"
#include "json/json.h"
#include "PushManager.hpp"

#define UNAVAILABLE_FD (-1)

PushHandler::PushHandler()
{
    //ctor
    m_pLogicHandler = NULL;
    m_iFd = UNAVAILABLE_FD;

    pthread_mutex_init(&m_mutexInactive, NULL);
}

PushHandler::~PushHandler()
{
    //dtor
    delete m_pLogicHandler;
    m_pLogicHandler = NULL;

    pthread_mutex_destroy(&m_mutexInactive);
}

bool PushHandler::Parsing(char *pBody, LPMESSAGE_DATA pMsgData)
{
    CAutoLock lock(&m_mutexInactive);

    gvLog(LOG_MSG, "(PushHandler::Parsing) MSG: locked iFd:%d", pMsgData->iFd);

    if (NULL == pBody || NULL == pMsgData || NULL == pMsgData->pData || NULL == pMsgData->pData->m_cRsp){
        gvLog(LOG_ERR_USER, "(PushHandler::Parsing) ERR: NULL == pBody || NULL == pMsgData || NULL == pMsgData->pData || NULL == pMsgData->pData->m_cRsp");
        return false;
    }

    ParsingURLString(pBody);

    if (0 == strncmp(JSONPARAM_SGIN, pBody, strlen(JSONPARAM_SGIN))){
        char *pEnd = strstr(pBody, BODY_END_SIGN);
        if (NULL != pEnd){
            *pEnd = '\0';
        }
        pBody += strlen(JSONPARAM_SGIN);
    }

    Json::Reader reader;
    Json::Value jsonRoot;
    bool parsingSuccessful = reader.parse(pBody, jsonRoot);
    if (!parsingSuccessful){
        gvLog(LOG_ERR_USER, "(PushHandler::Parsing) ERR: ParsingToJsonFail iFd:%d pBody:%s", pMsgData->iFd, pBody);
        return false;
    }

    gvLog(LOG_MSG, "(PushHandler::Parsing) MSG: iFd:%d pBody:\n%s\n", pMsgData->iFd, pBody);
    if (ClientHandler::CanHandle(jsonRoot, pBody)){
        if (NULL != m_pLogicHandler) {
            if (NULL == dynamic_cast<ClientHandler*>(m_pLogicHandler)){
                delete m_pLogicHandler;
                m_pLogicHandler = NULL;
            }
            else {
                m_pLogicHandler->Reset();
            }
        }

        if (NULL == m_pLogicHandler) {
            m_pLogicHandler = new ClientHandler();
            m_pLogicHandler->SetPushHandler(this);
        }
    }
    else if (ServerHandler::CanHandle(jsonRoot, pBody)){
        if (NULL != m_pLogicHandler) {
            if (NULL == dynamic_cast<ServerHandler*>(m_pLogicHandler)){
                delete m_pLogicHandler;
                m_pLogicHandler = NULL;
            }
            else {
                m_pLogicHandler->Reset();
            }
        }

        if (NULL == m_pLogicHandler) {
            m_pLogicHandler = new ServerHandler();
            m_pLogicHandler->SetPushHandler(this);
        }
    }

    if (NULL != m_pLogicHandler){
        return m_pLogicHandler->HandleProtocolData(jsonRoot, pMsgData);
    }

    gvLog(LOG_ERR_USER, "(PushHandler::Parsing) ERR: Cannot handle iFd:%d pBody:%s", pMsgData->iFd, pBody);
    return false;
}

bool PushHandler::SetInactive(bool bIsDeregisterTokenId)
{
    CAutoLock lock(&m_mutexInactive);

    gvLog(LOG_MSG, "(PushHandler::SetInactive) MSG: begin");
    if (NULL != m_pLogicHandler){
        gvLog(LOG_MSG, "(PushHandler::SetInactive) MSG: iFd:%d m_pLogicHandler:%p", m_iFd, m_pLogicHandler);
        m_pLogicHandler->Reset(bIsDeregisterTokenId);
        // 不用重置 m_iFd ，m_iFd就是对应一个PushHandler
    }
    gvLog(LOG_MSG, "(PushHandler::SetInactive) MSG: end");

    return true;
}

void PushHandler::SendSuccess()
{
    if (NULL != m_pLogicHandler){
        m_pLogicHandler->SendSuccess();
    }
}

void PushHandler::DataFormatError(LPMESSAGE_DATA pMsgData)
{
    Json::Value root;
    root[RET_PARAM] = FORMATERR_CODE;

    Json::FastWriter writer;
    string strJson = writer.write(root);

    snprintf(pMsgData->pData->m_cRsp, sizeof(pMsgData->pData->m_cRsp), HTTP_REPONSE_CLOSE, HTTP_200_OK, HTTP_TEXT, strJson.length());
    strcat(pMsgData->pData->m_cRsp, strJson.c_str());
    pMsgData->pData->m_usRspTotal = strlen(pMsgData->pData->m_cRsp);

    pMsgData->bSendAndClose = true;

    gvLog(LOG_MSG, "(PushHandler::DataFormatError) MSG:");
}

bool PushHandler::RegisterTokenId(const char *pTokenId)
{
    return PushManager::GetInstance()->AddToken(pTokenId, this);
}

bool PushHandler::DeregisterTokenId(const char *pTokenId)
{
    return PushManager::GetInstance()->RemoveToken(pTokenId);
}

bool PushHandler::SendPushMessage(const char *pPushMsg)
{
    ClientHandler *pClienHandler = dynamic_cast<ClientHandler*>(m_pLogicHandler);
    if (NULL != pClienHandler)
    {
        return pClienHandler->SendPushMessage(pPushMsg);
    }
    gvLog(LOG_ERR_USER, "(PushHandler::SendPushMessage) ERR: m_pLogicHandler is not ClientHandler");
    return false;
}

void PushHandler::SetFd(int iFd)
{
    gvLog(LOG_MSG, "(PushHandler::SetFd) MSG: iFd:%d", iFd);
    m_iFd = iFd;
}

int PushHandler::GetFd() const
{
    return m_iFd;
}

bool PushHandler::CharTo16Hex(const char cChar, char &cTrans)
{
    if ('0' <= cChar && cChar <= '9'){
        cTrans = cChar - '0';
        return true;
    }
    else if ('A' <= cChar && cChar <= 'F'){
        cTrans = cChar - 'A' + 10;
        return true;
    }
    else if ('a' <= cChar && cChar <= 'f'){
        cTrans = cChar - 'a' + 10;
        return true;
    }

    return false;
}

void PushHandler::ParsingURLString(char *pUrlString)
{
    if (NULL == pUrlString || '\0' == *pUrlString){
        return;
    }

    char *pStr = pUrlString;
    char *pWrite = pUrlString;
    char cAcsii = 0;
    char cAcsiiLow = 0;
    while ('\0' != *pStr){
        if ('%' == *pStr){
            if (CharTo16Hex(*(pStr+1), cAcsii )){
                cAcsii *= 16;

                if (CharTo16Hex(*(pStr+2), cAcsiiLow)){
                    cAcsii += cAcsiiLow;
                    *pWrite = cAcsii;

                    pWrite++;
                    pStr += 3;
                    continue;
                }
            }
        }
        else if ('+' == *pStr){
            *pWrite = ' ';
        }

        pStr++;
        pWrite++;
    }
    *pWrite = '\0';
}
