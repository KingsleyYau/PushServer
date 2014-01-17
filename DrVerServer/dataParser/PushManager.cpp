/*
 * File         : PushManager.cpp
 * Date         : 2012-06-19
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for handle logic
 */

#include "PushManager.hpp"
#include "PushHandler.hpp"
#include "DBHandler.hpp"
#include "ClientHandler.hpp"
#include "DrVerData.hpp"

#define IsAvailableIFD(IFD) (MAX_CONNECTION_COUNT > IFD)
#define IFDToIndex(IFD) (IFD - 1)

PushManager* PushManager::GetInstance()
{
    static PushManager instance;
    return &instance;
}

PushManager::PushManager()
{
    //ctor
    memset(m_aPushHandler, 0, sizeof(m_aPushHandler));
    pthread_rwlock_init(&m_rwlockTokenMap, NULL);
}

PushManager::~PushManager()
{
    //dtor
    pthread_rwlock_destroy(&m_rwlockTokenMap);
}

bool PushManager::AddToken(const char *pToken, PushHandler *pPushHandler)
{
    if (NULL == pToken || 0 == *pToken){
        return false;
    }

    gvLog(LOG_MSG, "(PushManager::AddToken) MSG: begin pToken:%s", pToken);
    pthread_rwlock_wrlock(&m_rwlockTokenMap);
    gvLog(LOG_MSG, "(PushManager::AddToken) MSG: in lock pToken:%s", pToken);
    pair< PushHandlerMap::iterator, bool > result;
    result = m_mapPushHandler.insert( PushHandlerMap::value_type(pToken, pPushHandler) );
    if (!result.second){
        // tokenid exist
        PushHandlerMap::iterator iter;
        iter = m_mapPushHandler.find(pToken);
        if (m_mapPushHandler.end() != iter){
            PushHandler* pOldPushHandler = iter->second;
            pthread_rwlock_unlock(&m_rwlockTokenMap);

            gvLog(LOG_MSG, "(PushManager::AddToken) MSG: reset pToken:%s iFd:%d", pToken, pOldPushHandler->GetFd());
            pOldPushHandler->SetInactive(false);

            pthread_rwlock_wrlock(&m_rwlockTokenMap);
        }

        m_mapPushHandler.erase(pToken);
        result = m_mapPushHandler.insert( PushHandlerMap::value_type(pToken, pPushHandler) );
    }
    gvLog(LOG_MSG, "(PushManager::AddToken) MSG: out lock end result.second:%d pToken:%s", result.second, pToken);
    pthread_rwlock_unlock(&m_rwlockTokenMap);

    return result.second;
}

bool PushManager::RemoveToken(const char *pToken)
{
    if (NULL == pToken){
        return false;
    }

    gvLog(LOG_MSG, "(PushManager::RemoveToken) MSG: begin pToken:%s", pToken);
    pthread_rwlock_wrlock(&m_rwlockTokenMap);
    gvLog(LOG_MSG, "(PushManager::RemoveToken) MSG: in lock pToken:%s", pToken);
    bool result = m_mapPushHandler.erase(pToken) > 0;
    gvLog(LOG_MSG, "(PushManager::RemoveToken) MSG: out lock pToken:%s", pToken);
    pthread_rwlock_unlock(&m_rwlockTokenMap);

    gvLog(LOG_MSG, "(PushManager::RemoveToken) MSG: pToken:%s", pToken);
    return result;
}

PushHandler* PushManager::GetPushHandlerWithToken(const char *pToken)
{
    if (NULL == pToken){
        return NULL;
    }

    PushHandler* pPushHandler = NULL;

    PushHandlerMap::iterator iter;
    pthread_rwlock_rdlock(&m_rwlockTokenMap);
    iter = m_mapPushHandler.find(pToken);
    if (m_mapPushHandler.end() != iter){
        pPushHandler = iter->second;
    }
    pthread_rwlock_unlock(&m_rwlockTokenMap);

    gvLog(LOG_MSG, "(PushManager::GetPushHandlerWithToken) MSG: pPushHandler:%p pToken:%s", pPushHandler, pToken);
    return pPushHandler;
}

void PushManager::SendPushMessage(const char *pTokenid, const char *pBody)
{
    PushHandler* pPushHandler = PushManager::GetInstance()->GetPushHandlerWithToken(pTokenid);
    if ( NULL == pPushHandler || !pPushHandler->SendPushMessage(pBody) )
    {
        // send fail, save to db
        DBHandler *pDBHandler = DBHandler::GetInstance();
        pDBHandler->InsertPushMessage(pTokenid, pBody);

        gvLog(LOG_MSG, "(PushManager::SendPushMessage) MSG: pDBHandler->InsertPushMessage() pTokenid:%s\n pBody:%s", pTokenid, pBody);
    }
    gvLog(LOG_MSG, "(PushManager::SendPushMessage) MSG: end");
}

bool PushManager::Handle(LPMESSAGE_DATA pMsgData, char *pBody)
{
    if (IsAvailableIFD(pMsgData->iFd)) {
        PushHandler **ppPushHandler = &m_aPushHandler[IFDToIndex(pMsgData->iFd)];
        if (NULL == *ppPushHandler){
            gvLog(LOG_MSG, "(PushManager::Handle) MSG: Create PushHandler iFd:%d", pMsgData->iFd);
            *ppPushHandler = new PushHandler();
            if (NULL != *ppPushHandler){
                gvLog(LOG_MSG, "(PushManager::Handle) MSG: Create PushHandler success iFd:%d", pMsgData->iFd);
                (*ppPushHandler)->SetFd(pMsgData->iFd);
            }
        }

        if (NULL != *ppPushHandler){
            gvLog(LOG_MSG, "(PushManager::Handle) MSG: Parsing iFd:%d pMsgData:%p", pMsgData->iFd, pMsgData);
            return (*ppPushHandler)->Parsing(pBody, pMsgData);
        }
        else {
            gvLog(LOG_ERR_SYS, "(PushManager::Handle) ERR: ppPushHandler is NULL");
        }
    }
    else {
        gvLog(LOG_ERR_SYS, "(PushManager::Handle) ERR: pMsgData->iFd < MAX_CONNECTION_COUNT");
    }

    return false;
}

void PushManager::SendFail(LPMESSAGE_DATA pMsg)
{
    gvLog(LOG_MSG, "(PushManager::SendFail) MSG: begin pMsg:%p", pMsg);
    if (IsAvailableIFD(pMsg->iFd)){
        if (0 != pMsg->cBuffer[0] && !pMsg->bDBData) {
            PushHandler *pPushHandler = m_aPushHandler[IFDToIndex(pMsg->iFd)];
            if (NULL != pPushHandler){
                pPushHandler->SetInactive();
            }

            pMsg->pData->m_cRsp[pMsg->pData->m_usRspTotal] = 0;
            gvLog(LOG_MSG, "(PushManager::SendFail) MSG: tokenid:%s pMsg:%p fd:%d body:%s", pMsg->cBuffer, pMsg, pMsg->iFd, pMsg->pData->m_cRsp);
        }
        else if (0 == pMsg->cBuffer[0]) {
            gvLog(LOG_MSG, "(PushManager::SendFail) MSG: 0==pMsg->cBuffer[0] fd:%d pMsg:%p body:%s", pMsg->iFd, pMsg, pMsg->pData->m_cRsp);
        }
    }
    else {
        /// ERROR 不应该有这种情况出现
        gvLog(LOG_ERR_USER, "(PushManager::SendFail) ERR: fd:%d pMsg:%p", pMsg->iFd, pMsg);
    }
}

void PushManager::SendSuccess(int iFd)
{
    if (IsAvailableIFD(iFd)){
        PushHandler *pPushHandler = m_aPushHandler[IFDToIndex(iFd)];
        if (NULL != pPushHandler){
            pPushHandler->SendSuccess();
        }
    }
}

bool PushManager::Close(int iFd)
{
    gvLog(LOG_MSG, "(PushManager::Close) MSG: iFd:%d m_aPushHandler[iFd]:%p", iFd, m_aPushHandler[IFDToIndex(iFd)]);
    if (IsAvailableIFD(iFd)){
        PushHandler *pPushHandler = m_aPushHandler[IFDToIndex(iFd)];
        if (NULL != pPushHandler){
            pPushHandler->SetInactive();
        }
    }
    return true;
}

void OnCloseSocket(int iFd)
{
    gvLog(LOG_MSG, "(OnCloseSocket) MSG: iFd:%d", iFd);
    PushManager::GetInstance()->Close(iFd);
}

int PushManager::GetOnlineDeviceCout()
{
    int iDeviceCount = 0;
    pthread_rwlock_wrlock(&m_rwlockTokenMap);
    iDeviceCount = m_mapPushHandler.size();
    pthread_rwlock_unlock(&m_rwlockTokenMap);
    return iDeviceCount;
}
