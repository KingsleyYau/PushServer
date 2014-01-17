/*
 * File         : RspParser.cpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server reponse parser
 */

#include "RspParser.hpp"
#include <linux/sockios.h>

extern MsgListManager* g_MsgListWait;
extern int g_iSendCount;

RspParser::RspParser()
{
}

RspParser::~RspParser()
{
}

int RspParser::ParserData(LPMESSAGE_DATA pMsg)
{
    if (pMsg && pMsg->pData && pMsg->pData->m_usRspTotal > 0) {
        if (pMsg->sTimes > 0) {
            int iSend = SendData(pMsg->iFd, pMsg->pData->m_cRsp + pMsg->pData->m_usRspSend, pMsg->pData->m_usRspTotal - pMsg->pData->m_usRspSend);
            gvLog(LOG_MSG, "(RspParser::ParserData) iSend:%d", iSend);
            if (iSend >= 0){
                pMsg->pData->m_usRspSend += iSend;
                if (pMsg->pData->m_usRspTotal > pMsg->pData->m_usRspSend) {
                    pMsg->sTimes--;
                    if (pMsg->sTimes > 0){
                        return 0;
                    }
                }
                else {
                    // check send buffer contant length
                    size_t ileftUnsend;
                    if(ioctl(pMsg->iFd, SIOCOUTQ, (char*)&ileftUnsend) >= 0) {
                        if (0 == ileftUnsend) {
                            /// 统计发出包数
                            g_iSendCount++;

                            gvLog(LOG_MSG, "(RspParser::ParserData) MSG: success iFd:%d pMsg:%p tokenid:%s msg:%s", pMsg->iFd, pMsg, pMsg->cBuffer, pMsg->pData->m_cRsp);
                            return pMsg->pData->m_usRspTotal;
                        }
                        else {
                            // push to wait list
                            pMsg->lLastSendTime = GetTickCount();
                            g_MsgListWait->Put_Msg(pMsg);
                            return -2;
                        }
                    }
                }
            }
        }
    }
    else
    {
        gvLog(LOG_ERR_USER, "(RspParser::ParserData) ERR: pMsg:%p pMsg->pData->m_usRspTotal:%d", pMsg, pMsg->pData->m_usRspTotal);
    }
    return -1;
}

int RspParser::SendData(int iFd, char* pData, int iLen)
{
    if (!pData || iLen <= 0) return -1;

    int iSend = 0;
    int iRet = -1;

    struct pollfd fds;
    memset(&fds, 0, sizeof(struct pollfd));
    fds.fd = iFd;
    fds.events = POLLOUT;
    fds.revents = 0;
    iRet = poll(&fds, 1, MAX_SEND_TIME);
    switch(iRet) {
        case -1:
            return -1;
        case 0:
            gvLog(LOG_WARNING, "(RspParser::SendData) waitting for send data timeout.");
            break;
        default:
            if (fds.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                gvLog(LOG_WARNING, "(RspParser::SendData) POLLNVAL | POLLERR | POLLHUP end.");
                return -1;
            } else if (fds.revents & POLLOUT) {
                iRet = send(iFd, pData, iLen, 0);
                if (iRet == 0) {
                    return -1;
                }
                if (iRet < 0) {
                    if (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN) {
                        gvLog(LOG_WARNING, "(RspParser::SendData) errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN end.");
                        return 0;
                    }
                    gvLog(LOG_WARNING, "(RspParser::SendData) iRet:%d end.", iRet);
                    return -1;
                }
                iSend += iRet;
            }
            break;
    }
    return iSend;
}
