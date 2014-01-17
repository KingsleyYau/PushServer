/*
 * File         : DBHandle.cpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for DB<->ClientHandler/ServerHandler
 */

#include "DBHandler.hpp"
#include "DBSpool.hpp"
#include <string>
#include "TimeProc.hpp"

// db table
#define PUSH_MESSAGE_TABLE          "push_message"
#define PUSH_IDENTIFICATION_TABLE   "push_identification"
#define CLIENT_INFO_TABLE           "client_info"
#define CLIENT_ONLINE_RECORD_TABLE  "client_online_record"

// push_message table field
#define PMT_ID_FIELD        "id"
#define PMT_TOKENID_FIELD   "tokenid"
#define PMT_MESSAGE_FIELD   "message"

// push_identification field
#define PIT_APPID_FIELD         "appid"
#define PIT_APPKEY_FIELD        "appkey"
#define PIT_APPLASTVER_FIELD    "applastver"
#define PIT_APPPKGURL_FIELD     "apppkgurl"
#define PIT_APPVERDESC_FIELD    "appverdesc"
#define PIT_UPDATESTATUS        "updatestatus"
#define PIT_TESTTOKENID         "testtokenid"

// client_info field
#define CIT_TOKENID_FIELD       "tokenid"
#define CIT_APPID_FIELD         "appid"
#define CIT_APPVER_FIELD        "appver"
#define CIT_MODEL_FIELD         "model"
#define CIT_SYSTEM_FIELD        "system"

// client_online_record field
#define CORT_TOKENID_FIELD      "tokenid"
#define CORT_IP_FIELD           "ip"
#define CORT_PORT_FIELD         "port"
#define CORT_START_FIELD        "start"
#define CORT_END_FIELD          "end"
#define CORT_OFFLINEMSG_FIELD   "offlinemsg"
#define CORT_ONLINEMSG_FIELD    "onlinemsg"

DBHandler* DBHandler::GetInstance()
{
    static DBHandler s_dbHandlerInstance;
    return &s_dbHandlerInstance;
}

DBHandler::DBHandler()
{
    //ctor
    m_pDBSpool = NULL;
}

DBHandler::~DBHandler()
{
    //dtor
    delete m_pDBSpool;
    m_pDBSpool = NULL;
}

bool DBHandler::InitDB(const DB_CONF &dbConfig)
{
    delete m_pDBSpool;
    m_pDBSpool = new DBSpool();

    bool bResult = false;
    if (NULL != m_pDBSpool){
        bResult = m_pDBSpool->SetConnection(dbConfig.m_usThreadPool);
        bResult = bResult && m_pDBSpool->SetDBparm(dbConfig.m_strServer, dbConfig.m_usPort, dbConfig.m_strDbName,
                                dbConfig.m_strUserName, dbConfig.m_strPass);
        bResult = bResult && m_pDBSpool->Connect();

        if (!bResult){
            gvLog(LOG_ERR_SYS, "(DBHandler::InitDB) ERR: Core Server Create Failed while connecting to database!");

            delete m_pDBSpool;
            m_pDBSpool = NULL;
        }
        else {
            // initial
            bResult = LoadPushIdentificationTable();
        }
    }
    return bResult;
}

bool DBHandler::Restart()
{
    return  LoadPushIdentificationTable();
}

/// ---------------- PushMessage operation ----------------
bool DBHandler::InsertPushMessage(const char *tokenid, const char *message)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_5K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "INSERT INTO %s(%s,%s) VALUES('%s','%s')"
                , PUSH_MESSAGE_TABLE, PMT_TOKENID_FIELD, PMT_MESSAGE_FIELD
                , tokenid, message);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_INSERT == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }

    if (!bResult){
        gvLog(LOG_ERR_USER, "(DBHandler::InsertPushMessage) ERR: m_pDBSpool is NULL");
    }
    return bResult;
}

PushMessageDBList DBHandler::GetPushMessageWithTokenId(const char *tokenid)
{
    PushMessageDBList pushMsgList;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_1K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "SELECT * FROM %s WHERE %s='%s' order by id"
                , PUSH_MESSAGE_TABLE, PMT_TOKENID_FIELD, tokenid);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        int iExecuteSqlResult = m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows);
        if (SQL_TYPE_SELECT == iExecuteSqlResult && iRows > 0)
        {
            int iFields = mysql_num_fields(pSQLRes);
            if (iFields == 0) {
                goto GETPUSHMSGERR;
            }

            MYSQL_FIELD* fields;
            fields = mysql_fetch_fields(pSQLRes);
            for (int i = 0; i < iRows; i++) {
                MYSQL_ROW row;
                if ((row = mysql_fetch_row(pSQLRes)) == NULL) {
                    break;
                }

                TPushMessageDBItem item;
                string strValue;
                for (int j = 0; j < iFields; j++) {
                    if (row[j]) {
                        strValue = row[j];
                    } else {
                        strValue = "";
                    }

                    if (strcmp(fields[j].name, PMT_ID_FIELD) == 0) {
                        item.id = atoi(strValue.c_str());
                    } else if(strcmp(fields[j].name, PMT_TOKENID_FIELD) == 0) {
                        item.tokenid = strValue;
                    } else if(strcmp(fields[j].name, PMT_MESSAGE_FIELD) == 0) {
                        item.message = strValue;
                    }
                }
                pushMsgList.push_back(item);
            }
        }
        else if (SQL_TYPE_SELECT != iExecuteSqlResult){
            gvLog(LOG_ERR_SYS, "(DBHandler::GetPushMessageWithTokenId) ERR: ExecuteSQL error!");
        }

GETPUSHMSGERR:
        m_pDBSpool->ReleaseConnection(shIdt);
    }

    return pushMsgList;
}

bool DBHandler::RemovePushMessage(int id)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_1K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "DELETE FROM %s WHERE %s=%d"
                , PUSH_MESSAGE_TABLE, PMT_ID_FIELD, id);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_DELETE == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }
    else {
        gvLog(LOG_MSG, "(DBHandler::RemovePushMessage) m_pDBSpool is NULL");
    }
    return bResult;
}

/// ---------------- PushIdentification operation ----------------
bool DBHandler::InsertPushIdentification(const char *appid, const char *appkey)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_1K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "INSERT INTO %s(%s,%s) VALUES('%s','%s')"
                , PUSH_IDENTIFICATION_TABLE, PIT_APPID_FIELD, PIT_APPKEY_FIELD
                , appid, appkey);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_INSERT == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }
    else {
        gvLog(LOG_MSG, "(DBHandler::InsertPushIdentification) m_pDBSpool is NULL");
    }
    return bResult;
}

TPushIdentItem DBHandler::GetAppInfoWithAppid(const char *appid)
{
    TPushIdentItem item;
    PushIdentMap::iterator iter = m_mapPushIdent.find(appid);
    if (m_mapPushIdent.end() != iter){
        item = iter->second;
    }
    gvLog(LOG_MSG, "(DBHandler::GetAppInfoWithAppid) appid:%s, appkey:%s, applastver:%s, apppkgurl:%s"
            , item.appid.c_str(), item.appkey.c_str(), item.applastver.c_str(), item.apppkgurl.c_str());
    return item;
}

bool DBHandler::RemovePushIdentification(const char *appid)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_1K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "DELETE FROM %s WHERE %s='%s'"
                , PUSH_IDENTIFICATION_TABLE, PIT_APPID_FIELD, appid);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_DELETE == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }
    else {
        gvLog(LOG_MSG, "(DBHandler::RemovePushIdentification) m_pDBSpool is NULL");
    }
    return bResult;
}

bool DBHandler::LoadPushIdentificationTable()
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_1K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "SELECT * FROM %s", PUSH_IDENTIFICATION_TABLE);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        if (SQL_TYPE_SELECT == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows)
            && iRows > 0)
        {
            int iFields = mysql_num_fields(pSQLRes);
            if (iFields == 0) {
                goto GETPUSHMSGERR;
            }

            m_mapPushIdent.erase(m_mapPushIdent.begin(), m_mapPushIdent.end());
            MYSQL_FIELD* fields;
            MYSQL_ROW row;
            fields = mysql_fetch_fields(pSQLRes);
            for (int i = 0; i < iRows; i++) {
                if ((row = mysql_fetch_row(pSQLRes)) == NULL) {
                    break;
                }

                TPushIdentItem item;
                string strValue;
                for (int j = 0; j < iFields; j++) {
                    if (row[j]) {
                        strValue = row[j];
                    } else {
                        strValue = "";
                    }

                    if(strcmp(fields[j].name, PIT_APPID_FIELD) == 0) {
                        item.appid = strValue;
                    } else if(strcmp(fields[j].name, PIT_APPKEY_FIELD) == 0) {
                        item.appkey = strValue;
                    } else if (strcmp(fields[j].name, PIT_APPLASTVER_FIELD) == 0) {
                        item.applastver = strValue;
                    } else if (strcmp(fields[j].name, PIT_APPPKGURL_FIELD) == 0) {
                        item.apppkgurl = strValue;
                    } else if (strcmp(fields[j].name, PIT_APPVERDESC_FIELD) == 0) {
                        item.appverdesc = strValue;
                    } else if (strcmp(fields[j].name, PIT_UPDATESTATUS) == 0) {
                        item.updatestatus = atoi(strValue.c_str());
                    } else if (strcmp(fields[j].name, PIT_TESTTOKENID) == 0) {
                        item.testtokenid = strValue;
                    }
                }
                m_mapPushIdent.insert(PushIdentMap::value_type(item.appid, item));
                gvLog(LOG_MSG, "(DBHandler::LoadPushIdentificationTable) appid:%s, appkey:%s, applastver:%s, apppkgurl:%s, appverdesc:%s updatestatus:%d testtokenid:%s"
                      , item.appid.c_str(), item.appkey.c_str(), item.applastver.c_str(), item.apppkgurl.c_str(), item.appverdesc.c_str(), item.updatestatus, item.testtokenid.c_str());
            }
            bResult = true;
        }
GETPUSHMSGERR:
        m_pDBSpool->ReleaseConnection(shIdt);
    }

    if (!bResult){
        gvLog(LOG_ERR_SYS, "(DBHandler::LoadPushIdentificationTable) ExecuteSQL error!");
    }
    return bResult;
}

/// ---------------- ClientInfo operation ----------------
bool DBHandler::ReplaceClientInfo(const TClientInfoItem &item)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cSQLBuffer[BUFFER_SIZE_2K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "REPLACE INTO %s SET %s='%s',%s='%s',%s='%s',%s='%s',%s='%s'"
                , CLIENT_INFO_TABLE
                , CIT_TOKENID_FIELD, item.tokenid.c_str()
                , CIT_APPID_FIELD, item.appid.c_str()
                , CIT_APPVER_FIELD, item.appver.c_str()
                , CIT_MODEL_FIELD, item.model.c_str()
                , CIT_SYSTEM_FIELD, item.system.c_str());

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_REPLACE == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }

    if (!bResult){
        gvLog(LOG_ERR_USER, "(DBHandler::ReplaceClientInfo) ERR: m_pDBSpool is NULL");
    }
    return bResult;
}

/// ---------------- ClientOnlineRecord operation ----------------
bool DBHandler::InsertClientOnlineRecord(const TOnlineRecordItem &item)
{
    bool bResult = false;
    if (NULL != m_pDBSpool){
        char cStart[32];
        GetLocalTimeString(cStart, sizeof(cStart), item.start);

        char cEnd[32];
        GetLocalTimeString(cEnd, sizeof(cEnd), item.end);

        char cSQLBuffer[BUFFER_SIZE_2K];
        snprintf(cSQLBuffer, sizeof(cSQLBuffer), "INSERT INTO %s(%s,%s,%s,%s,%s,%s,%s) VALUES('%s','%s',%d,'%s','%s',%d,%d)"
                , CLIENT_ONLINE_RECORD_TABLE
                , CORT_TOKENID_FIELD, CORT_IP_FIELD, CORT_PORT_FIELD, CORT_START_FIELD, CORT_END_FIELD, CORT_OFFLINEMSG_FIELD, CORT_ONLINEMSG_FIELD
                , item.tokenid.c_str(), item.ip.c_str(), item.port, cStart, cEnd, item.offlineMsg, item.onlineMsg);

        MYSQL_RES* pSQLRes = NULL;
        short shIdt = 0;
        int iRows = 0;
        bResult = (SQL_TYPE_INSERT == m_pDBSpool->ExecuteSQL(cSQLBuffer, &pSQLRes, shIdt, iRows));
        m_pDBSpool->ReleaseConnection(shIdt);
    }

    if (!bResult){
        gvLog(LOG_ERR_USER, "(DBHandler::InsertClientOnlineRecord) ERR: m_pDBSpool is NULL");
    }
    return bResult;
}
