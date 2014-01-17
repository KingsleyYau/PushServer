/*
 * File         : RecvParser.cpp
 * Date         : 2011-12-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server receive parser
 */

#include "RecvParser.hpp"

extern int g_iRecvCount;

RecvParser::RecvParser()
{
    m_iRev = 0;
    m_iLen = 0;
}

RecvParser::~RecvParser()
{
}

int RecvParser::ParserData(LPMESSAGE_DATA pMsg)
{
    if (!pMsg || !(pMsg->pData)) return -1;

    //do NOT accept new connect while recv len == 0
    ioctl(pMsg->iFd, FIONREAD, &m_iRev);
    if (m_iRev < 0 || m_iRev >= BUFFER_SIZE_10K) {
        gvLog(LOG_WARNING, "(RecvParser::ParserData) m_iRev:%d end.", m_iRev);
        return -1;
    }

    int m_iLen = recv(pMsg->iFd, pMsg->pData->m_cRsp, m_iRev, 0);
    if (m_iLen < 0) {
        if (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN) {
			gvLog(LOG_WARNING, "(RecvParser::ParserData) errno == EWOULDBLOCK end.");
			return 0;
		}
		gvLog(LOG_WARNING, "(RecvParser::ParserData) socket(%d) error end.", pMsg->iFd);
    } else if (m_iLen == 0) {
        gvLog(LOG_WARNING, "(RecvParser::ParserData) remote socket(%d) close.", pMsg->iFd);
    } else {
//        if (pMsg->pData->m_cRsp[m_iLen - 1] == '\n' && pMsg->pData->m_cRsp[m_iLen - 2] == '\r' &&
//            pMsg->pData->m_cRsp[m_iLen - 3] == '\n' && pMsg->pData->m_cRsp[m_iLen - 4] == '\r') {
            /// 统计接收包数
            g_iRecvCount++;

            pMsg->pData->m_usRspTotal = m_iLen;
            pMsg->sTimes = MAX_SEND_TIMES;
            return m_iLen;
//        }
//        gvLog(LOG_WARNING, "(RecvParser::ParserData) can not get http end data from recv buffer.");
    }
    return -1;
}
