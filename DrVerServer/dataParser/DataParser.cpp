/*
 * File         : DataParser.cpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server data parser
 */

#include <time.h>
#include "DataParser.hpp"
#include "md5.h"
#include "PushManager.hpp"
#include "PushHandler.hpp"

extern SYS_CONF g_SysConf;

DataParser::DataParser()
{
    m_pPushHandler = NULL;
//    m_pVerDBMapping = NULL;
    ReSetStr();
}

DataParser::~DataParser()
{
}

//bool DataParser::InitVerDBMapping(VerDBMapping *pVerDBMapping)
//{
////    if (pVerDBMapping){
////        m_pVerDBMapping = pVerDBMapping;
////        return true;
////    }
////    return false;
//}

bool DataParser::InitPushHandler(PushHandler *pPushHandler)
{
    if (pPushHandler){
        m_pPushHandler = pPushHandler;
        return true;
    }
    return false;
}

int DataParser::ParserData(LPMESSAGE_DATA pMsg)
{
    if (!pMsg || !(pMsg->pData)) return -1;

    gvLog(LOG_MSG, "(DataParser::ParserData) MSG: begin");

    LPDRVER_DATA pData = pMsg->pData;
    m_iFd = pMsg->iFd;

    pData->m_cRsp[pData->m_usRspTotal] = '\0';
    char *pBody = strstr(pData->m_cRsp, END_WITH_HTTP);

    if (NULL != pBody && strlen(pBody) > strlen(END_WITH_HTTP)){
        *pBody = '\0';
        UpCase(pData->m_cRsp, (int)(pBody - pData->m_cRsp));

        if (NULL != strstr(pData->m_cRsp, HTTP_KEEPALIVE)){
            pBody += strlen(END_WITH_HTTP);
            if (PushManager::GetInstance()->Handle(pMsg, pBody)){
                return 0;
            }
        }
        else {
            gvLog(LOG_ERR_USER, "(DataParser::ParserData) ERR: %s not found", HTTP_KEEPALIVE);
        }
    }

    // data format error
    gvLog(LOG_ERR_USER, "(DataParser::ParserData) ERR: pData->m_cRsp:\r\n%s\r\n\r\npBody:\r\n%s\r\n\r\n", pData->m_cRsp, (NULL != pBody ? pBody : ""));
    PushHandler::DataFormatError(pMsg);
    return 0;
}

void DataParser::ReSetStr()
{
    m_strTime = "";
    m_strType = "";
    m_strKey = "";
    m_strHash = "";
    m_strVer = "";
    m_strRnd = "";
    m_strChk = "";
    m_bLeak100004 = false;
}

string DataParser::MakeMD5(string str)
{
    md5_state_t ctx;
    memset(m_cBuffer, 0, sizeof(m_cBuffer));
    md5_init(&ctx);
    md5_append(&ctx, (const unsigned char*)str.c_str(), str.length());
    md5_finish(&ctx, (unsigned char*)m_cBuffer);

    for (int i = 12, j = 0; i < 16; i++, j++){
        sprintf(m_cBuffer + 16 + j * 2, "%02x", (unsigned char)m_cBuffer[i]);
    }
    m_strTemp_N = (char*)(m_cBuffer + 16);

    gvLog(LOG_WARNING, "(DataParser::MakeMD5) md5(%s)=%s", str.c_str(), m_strTemp_N.c_str());
    return m_strTemp_N;
}

string DataParser::MakeCRC(string str)
{
    unsigned long ui = m_Arithmetic.MakeCRC32((char*)str.c_str(), str.length());
    memset(m_cBuffer, 0, sizeof(m_cBuffer));
    snprintf(m_cBuffer, sizeof(m_cBuffer), "%u", ui);
    m_strTemp_N = m_cBuffer;

    gvLog(LOG_WARNING, "(DataParser::MakeCRC) crc32(%s)=%s[%02x]", str.c_str(), m_strTemp_N.c_str(), ui);
    return m_strTemp_N;
}

bool DataParser::UrlChk()
{
    bool bUrlChk = false;
    if (m_strType == HTTP_TYPE_0000) {
        if (m_bLeak100004) {
            //for recover leak 100004
            m_strTemp_O = m_strTime + m_strHash + m_strKey;
        } else {
            m_strTemp_O = m_strTime + m_strHash + m_strVer + m_strKey;
        }
        if (m_strChk == MakeMD5(m_strTemp_O)) {
            bUrlChk = true;
        }
    } else {
        if (m_bLeak100004) {
            //for recover leak 100004
            m_strTemp_O = m_strTime + m_strHash + m_strKey + m_strRnd;
        } else {
            m_strTemp_O = m_strTime + m_strHash + m_strVer + m_strKey + m_strRnd;
        }
        if (m_strType == HTTP_TYPE_0003) {
            if (m_strChk == MakeCRC(m_strTemp_O)) {
                bUrlChk = true;
            }
        } else if (m_strType == HTTP_TYPE_0006) {
            if (m_strChk == MakeMD5(m_strTemp_O)) {
                bUrlChk = true;
            }
        }
    }

    if (!bUrlChk) {
        gvLog(LOG_WARNING, "(DataParser::UrlChk) check url fail");
    }
    return bUrlChk;
}

bool DataParser::TimeChk()
{
    //get remove UTC time_t
    struct tm rTm;
    rTm.tm_year = atoi(m_strTime.substr(0, 4).c_str()) - 1900;
    rTm.tm_mon = atoi(m_strTime.substr(4,2).c_str()) - 1;
    rTm.tm_mday = atoi(m_strTime.substr(6,2).c_str());
    rTm.tm_hour = atoi(m_strTime.substr(8,2).c_str());
    rTm.tm_min = atoi(m_strTime.substr(10,2).c_str());
    rTm.tm_sec = atoi(m_strTime.substr(12,2).c_str());
    time_t rTime = mktime(&rTm);

    //get local UTC time_t
    time_t lTime = time(NULL);
    struct tm lTm;
    gmtime_r(&lTime, &lTm);
    lTime = mktime(&lTm);

    //5 minutes limit
    if (abs(rTime - lTime) < TIME_DIFFERENCE) {
        return true;
    }
    return false;
}

void DataParser::SocketInfo(int iFd, char* pData)
{
    struct sockaddr_storage serv_addr;
    socklen_t addrlen = (socklen_t)sizeof(serv_addr);
    memset(m_cBuffer, 0, sizeof(m_cBuffer));

    getpeername(iFd, (struct sockaddr*)&serv_addr, (socklen_t*)&addrlen);
    gvLog(LOG_ERR_USER, "(DataParser::SocketInfo) from [%s:%d](%d) request %s", j_inet_ntop(&serv_addr, m_cBuffer, sizeof(m_cBuffer)), j_inet_getport(&serv_addr), iFd, pData);
}

int DataParser::HttpParser(char* pData)
{
    char *p = strstr(pData, END_WITH_LINE);
    p -= 9;//the len of (" http/1.0")

    int iEnd = (int)(p - pData);
    if (iEnd < 0 || iEnd > BUFFER_SIZE_2K){
        return 0;
    }
    *p = '\0';

    //make upper the 'GET /DRCLIENT/UPDATE?' and compare
    string strTmp = HTTP_GET;
    strTmp = strTmp + HTTP_UPDATE;
    UpCase(pData, strTmp.length());

    if (pData[0] != 'G' || pData[1] != 'E' || pData[2] != 'T' || pData[3] != ' '){
        gvLog(LOG_WARNING, "(DataParser::HttpParser) do not match http get request!");
        return 0;
    }

    int iPos;
    p = strstr(pData, HTTP_UPDATE);
    if (p){
        iPos = (int)(p + string(HTTP_UPDATE).length() - pData);
        if (iPos > 0) {
            ReSetStr();
            return HttpGetParam(pData, iPos, iEnd);
        } else {
            gvLog(LOG_WARNING, "(DataParser::HttpParser) get url fail.");
            return 0;
        }
    }
    gvLog(LOG_WARNING, "(DataParser::HttpParser) can not found key word %s.", HTTP_UPDATE);

    return 0;
}

int DataParser::HttpGetParam(char* pData, int iPos, int iEnd)
{
    SocketInfo(m_iFd, pData + iPos);
    //gvLog(LOG_MSG, "(DataParser::HttpGetParam) request %s", pData + iPos);

    char* pBuffer = pData + iPos;
    char *token = NULL, *poken = NULL, *token_next = NULL, *poken_next = NULL;
    bool bEleTime = false, bEleType = false, bEleKey = false, bEleHash = false, bEleVer = false, bEleRnd = false, bEleChk = false;
    int i = 0;

    token = strtok_r(pBuffer, "&", &token_next);
    while (token != NULL) {
        memset(m_cBuffer, 0, sizeof(m_cBuffer));
        strcpy(m_cBuffer, token);
        i = 0;

        //do param like time=20100517114367
        poken = strtok_r(m_cBuffer, "=", &poken_next);
        while (poken != NULL) {
            //do param like time and 20100517114367
            if ((i % 2) == 0){
                bEleTime = false, bEleType = false, bEleKey = false, bEleHash = false, bEleVer = false, bEleRnd = false, bEleChk = false;
                if (strcasecmp(poken, HTTP_ELE_TIME) == 0) {
                    bEleTime = true;
                } else if (strcasecmp(poken, HTTP_ELE_TYPE) == 0) {
                    bEleType = true;
                } else if (strcasecmp(poken, HTTP_ELE_KEY) == 0) {
                    bEleKey = true;
                } else if (strcasecmp(poken, HTTP_ELE_HASH) == 0) {
                    bEleHash = true;
                } else if (strcasecmp(poken, HTTP_ELE_VER) == 0) {
                    bEleVer = true;
                } else if (strcasecmp(poken, HTTP_ELE_RND) == 0) {
                    bEleRnd = true;
                } else if (strcasecmp(poken, HTTP_ELE_CHK) == 0) {
                    bEleChk = true;
                }
            }else if ((i % 2) == 1){
                if (bEleTime) {
                    m_strTime = poken;
                } else if (bEleType) {
                    m_strType = poken;
                    if ((m_strType != HTTP_TYPE_0000) && (m_strType != HTTP_TYPE_0003) && (m_strType != HTTP_TYPE_0006)) {
                        m_strType = "";
                    }
                } else if (bEleKey) {
                    m_strKey = poken;
                } else if (bEleHash) {
                    m_strHash = poken;
                } else if (bEleVer) {
                    m_strVer = poken;
                } else if (bEleRnd) {
                    m_strRnd = poken;
                } else if (bEleChk) {
                    m_strChk = poken;
                }
            }
            i++;
            poken = strtok_r(NULL, "=", &poken_next);
        }
        token = strtok_r(NULL, "&", &token_next);
    }

    gvLog(LOG_MSG, "(DataParser::HttpGetParam) trans time=%s type=%s key=%s hash=%s ver=%s rnd=%s chk=%s", m_strTime.c_str(), m_strType.c_str(), m_strKey.c_str(), m_strHash.c_str(),
                    m_strVer.c_str(), m_strRnd.c_str(), m_strChk.c_str());

    //for recover leak 100004
    if (m_strVer.length() < 1) {
        if (g_SysConf.m_Leak.m_b100004) {
            m_strVer = LEAK_100004;
            m_bLeak100004 = true;
            gvLog(LOG_ERR_USER, "(DataParser::HttpGetParam) recovered leak 100004, mark version %s", m_strVer.c_str());
        }
    }

    if (m_strTime.length() == 14 && m_strType.length() == 4 && m_strHash.length() == 32 &&
        m_strVer.length() > 16 && m_strRnd.length() == 6 && m_strChk.length() >= 1) {
        return 1;
    }
    gvLog(LOG_WARNING, "(DataParser::HttpGetParam) http request parser error.");
    return 0;
}
