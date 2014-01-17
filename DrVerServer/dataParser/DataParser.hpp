/*
 * File         : DataParser.hpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server data parser
 */

#ifndef _INC_DATAPARSER_
#define _INC_DATAPARSER_

#include "MessageMgr.hpp"
#include "Arithmetic.hpp"
//#include "VerDBMapping.hpp"

class PushHandler;
class DataParser
{
    public:
        DataParser();
        ~DataParser();

//        bool InitVerDBMapping(VerDBMapping *pVerDBMapping);
        bool InitPushHandler(PushHandler *pPushHandler);
        int ParserData(LPMESSAGE_DATA pMsg);

    protected:
        inline void ReSetStr();
        inline string MakeMD5(string str);
        inline string MakeCRC(string str);
        inline bool UrlChk();
        inline bool TimeChk();
        inline void SocketInfo(int iFd, char* pData);
        inline int HttpParser(char* pData);
        inline int HttpGetParam(char* pData, int iPos, int iEnd);

    protected:
//        VerDBMapping* m_pVerDBMapping;
        PushHandler*    m_pPushHandler;
        string m_strTime, m_strType, m_strKey, m_strHash, m_strVer, m_strRnd, m_strChk;
        string m_strTemp_O, m_strTemp_N;
        char m_cBuffer[BUFFER_SIZE_10K];
        int m_iTempTotal, m_iFd;
        Arithmetic m_Arithmetic;
        bool m_bLeak100004;
};

#endif
