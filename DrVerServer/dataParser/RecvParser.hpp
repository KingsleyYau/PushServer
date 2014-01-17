/*
 * File         : RecvParser.hpp
 * Date         : 2011-12-27
 * Author       : Keqin Su 
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server receive parser
 */

#ifndef _INC_RECVPARSER_
#define _INC_RECVPARSER_

#include "MessageMgr.hpp"

class RecvParser
{
    public:
        RecvParser();
        ~RecvParser();

        int ParserData(LPMESSAGE_DATA pMsg);
    
    protected:
        int m_iRev, m_iLen;
};

#endif
