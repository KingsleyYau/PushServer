/*
 * File         : RspParser.hpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server reponse parser
 */

#ifndef _INC_RSPPARSER_
#define _INC_RSPPARSER_

#include "MessageMgr.hpp"
#include "TimeProc.hpp"

class RspParser
{
    public:
        RspParser();
        ~RspParser();

        int ParserData(LPMESSAGE_DATA pMsg);

    protected:
        int SendData(int iFd, char* pData, int iLen);
};

#endif
