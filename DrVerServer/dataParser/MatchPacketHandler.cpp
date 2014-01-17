/*
 * File         : MatchPacketHandler.cpp
 * Date         : 2012-06-21
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for match packet handling
 */

#include "MatchPacketHandler.hpp"
#include "PushHandler.hpp"

#define HttpHeaderBreakSign "\r\n\r\n"
#define TransToIndex(iFd)       (iFd - 1)

MatchPacketHandler *MatchPacketHandler::GetInstance()
{
    static MatchPacketHandler matchPacketHandler;
    return &matchPacketHandler;
}

MatchPacketHandler::MatchPacketHandler()
{
    //ctor
    memset(&m_aMsgData, 0, sizeof(m_aMsgData));
}

MatchPacketHandler::~MatchPacketHandler()
{
    //dtor
}

LPMESSAGE_DATA MatchPacketHandler::Handling(LPMESSAGE_DATA pMsgData, LPMESSAGE_DATA &pErrMsgData)
{
    pErrMsgData = NULL;
    LPMESSAGE_DATA pOkData = NULL;
    bool bError = false;
    int index = TransToIndex(pMsgData->iFd);
    if (IsAvailableIndex(index)){
        if (NULL == m_aMsgData[index]){
            m_aMsgData[index] = pMsgData;
            if (!(GetContextLength(pMsgData, pMsgData->nHeaderLength, pMsgData->nContextLength)
                && BUFFER_SIZE_10K >= pMsgData->nHeaderLength + pMsgData->nContextLength))
            {
                gvLog(LOG_WARNING, "(MatchPacketHandler::Handling) ERR: iFd:%d pMsgData->nHeaderLength:%d\npMsgData->nContextLength:%d"
                  , pMsgData->iFd, pMsgData->nHeaderLength, pMsgData->nContextLength);
                bError = true;
            }
        }
        else if (m_aMsgData[index]->pData->m_usRspTotal + pMsgData->pData->m_usRspTotal
                 <= m_aMsgData[index]->nContextLength + m_aMsgData[index]->nHeaderLength)
        {
            strncpy(m_aMsgData[index]->pData->m_cRsp + m_aMsgData[index]->pData->m_usRspTotal
                   , pMsgData->pData->m_cRsp
                   , pMsgData->pData->m_usRspTotal);
            m_aMsgData[index]->pData->m_usRspTotal += pMsgData->pData->m_usRspTotal;

            PutIdleMsgBuff(pMsgData);
        }
        else {
            gvLog(LOG_WARNING, "(MatchPacketHandler::Handling) ERR: iFd:%d m_aMsgData[index]->pData->m_usRspTotal:%d\npMsgData->pData->m_usRspTotal:%d\nm_aMsgData[index]->nContextLength:%d\nm_aMsgData[index]->nHeaderLength:%d"
                  , pMsgData->iFd, m_aMsgData[index]->pData->m_usRspTotal, pMsgData->pData->m_usRspTotal, m_aMsgData[index]->nContextLength, m_aMsgData[index]->nHeaderLength);

            PutIdleMsgBuff(pMsgData);
            bError = true;
        }
    }
    else {
        gvLog(LOG_ERR_SYS, "(MatchPacketHandler::Handling) ERR: error MAX_CONNECTION_COUNT < pMsgData->iFd:%d", pMsgData->iFd);
        PutIdleMsgBuff(pMsgData);
        return NULL;
    }

    if (bError){
        gvLog(LOG_ERR_USER, "(MatchPacketHandler::Handling) ERR: packet error iFd:%d\r\n\r\nm_aMsgData:\r\n%s\r\n\r\npMsgData:%s\r\n\r\n"
              , pMsgData->iFd
              , NULL != m_aMsgData[index] && NULL != m_aMsgData[index]->pData->m_cRsp? m_aMsgData[index]->pData->m_cRsp : ""
              , NULL != pMsgData->pData->m_cRsp ? pMsgData->pData->m_cRsp : "");

        pErrMsgData = m_aMsgData[index];
        m_aMsgData[index] = NULL;
        if (NULL != pErrMsgData) {
            PushHandler::DataFormatError(pErrMsgData);
        }
        else {
            gvLog(LOG_ERR_USER, "(MatchPacketHandler::Handling) ERR: iFd:%d\nm_aMsgData[index]:%p\npErrMsgData:%p\npMsgData:%p\r\n"
            ,pMsgData->iFd , m_aMsgData[index], pErrMsgData, pMsgData);
        }
    }
    else if(NULL != m_aMsgData[index]
        && m_aMsgData[index]->nContextLength + m_aMsgData[index]->nHeaderLength == m_aMsgData[index]->pData->m_usRspTotal)
    {
        gvLog(LOG_MSG, "(MatchPacketHandler::Handling) MSG: Success iFd:%d", pMsgData->iFd);
        pOkData = m_aMsgData[index];
        m_aMsgData[index] = NULL;
        pOkData->pData->m_cRsp[pOkData->pData->m_usRspTotal] = '\0';
    }
    else if(NULL != m_aMsgData[index]
        && m_aMsgData[index]->nContextLength + m_aMsgData[index]->nHeaderLength < m_aMsgData[index]->pData->m_usRspTotal)
    {
        /// ERROR 不应该有这种情况出现
        gvLog(LOG_ERR_USER, "(MatchPacketHandler::Handling) ERR: iFd:%d\nm_aMsgData[index]->pData->m_usRspTotal:%d\npMsgData->pData->m_usRspTotal:%d\nm_aMsgData[index]->nContextLength:%d\nm_aMsgData[index]->nHeaderLength:%d\nm_aMsgData[index]->pData->m_cRsp:%s\n"
            ,pMsgData->iFd , m_aMsgData[index]->pData->m_usRspTotal, pMsgData->pData->m_usRspTotal, m_aMsgData[index]->nContextLength, m_aMsgData[index]->nHeaderLength, m_aMsgData[index]->pData->m_cRsp);

        pErrMsgData = m_aMsgData[index];
        m_aMsgData[index] = NULL;
        PushHandler::DataFormatError(pErrMsgData);
    }
    else {
        gvLog(LOG_MSG, "(MatchPacketHandler::Handling) MSG: \nNot enough: iFd:%d\nm_aMsgData[index]->pData->m_usRspTotal:%d\npMsgData->pData->m_usRspTotal:%d\nm_aMsgData[index]->nContextLength:%d\nm_aMsgData[index]->nHeaderLength:%d\nm_aMsgData[index]->pData->m_cRsp:\n%s"
            ,pMsgData->iFd , m_aMsgData[index]->pData->m_usRspTotal, pMsgData->pData->m_usRspTotal, m_aMsgData[index]->nContextLength, m_aMsgData[index]->nHeaderLength, m_aMsgData[index]->pData->m_cRsp);
    }
    return pOkData;
}

bool MatchPacketHandler::GetContextLength(LPMESSAGE_DATA pMsgData, unsigned int &nHeaderLength, unsigned int &nContextLength)
{
    const char *pHttpHeader = strstr(pMsgData->pData->m_cRsp, HttpHeaderBreakSign);
    if (NULL != pHttpHeader
        && (pHttpHeader - pMsgData->pData->m_cRsp) < pMsgData->pData->m_usRspTotal)
    {
        UpCase(pMsgData->pData->m_cRsp, pHttpHeader - pMsgData->pData->m_cRsp);

        const char *pContextLength = strstr(pMsgData->pData->m_cRsp, HTTP_CONTENTLENGTH);
        if (NULL != pContextLength
            && (pContextLength - pMsgData->pData->m_cRsp) < pMsgData->pData->m_usRspTotal)
        {
            pContextLength += strlen(HTTP_CONTENTLENGTH);
            const char *pContextLengthEnd = strstr(pContextLength, "\r\n");

            if (pContextLengthEnd - pContextLength < sizeof(pMsgData->pData->m_cBuff16)){
                int length = pContextLengthEnd - pContextLength;
                strncpy(pMsgData->pData->m_cBuff16, pContextLength, length);
                pMsgData->pData->m_cBuff16[length] = 0;
                nContextLength = (unsigned int)atoi(pMsgData->pData->m_cBuff16);
                nHeaderLength = (pHttpHeader - pMsgData->pData->m_cRsp) + strlen(HttpHeaderBreakSign);

                return true;
            }
        }
    }
    return false;
}
