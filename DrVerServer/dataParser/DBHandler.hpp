/*
 * File         : DBHandle.hpp
 * Date         : 2012-06-20
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for DB<->ClientHandler/ServerHandler
 */

#ifndef DBHANDLER_H
#define DBHANDLER_H

#include "DrVerData.hpp"
#include <string>
#include <list>
#include <map>
#include "ClientManager.hpp"

typedef struct _tagPushMessageDBItem{
    int id;
    string tokenid;
    string message;
    string ver;
    string updateUrl;
    _tagPushMessageDBItem(){}
    _tagPushMessageDBItem(const _tagPushMessageDBItem &item)
    {
        id = item.id;
        tokenid = item.tokenid;
        message = item.message;
        ver = item.ver;
        updateUrl = item.updateUrl;
    }
}TPushMessageDBItem;
typedef list<TPushMessageDBItem> PushMessageDBList;

enum {
    UPDATESTATUS_OFF = 0,   // ¹Ø±Õupdate
    UPDATESTATUS_OPEN = 1,  // ¹«¿ªupdate
    UPDATESTATUS_TEST = 2,  // ²âÊÔupdate
};

typedef struct _tagPushIdentItem{
    string appid;
    string appkey;
    string applastver;
    string apppkgurl;
    string appverdesc;
    int updatestatus;
    string testtokenid;
    _tagPushIdentItem()
    {
        updatestatus = UPDATESTATUS_OFF;
    }

    _tagPushIdentItem(const _tagPushIdentItem &item)
    {
        appid = item.appid;
        appkey = item.appkey;
        applastver = item.applastver;
        apppkgurl = item.apppkgurl;
        appverdesc = item.appverdesc;
        updatestatus = item.updatestatus;
        testtokenid = item.testtokenid;
    }
}TPushIdentItem;
typedef map<string, TPushIdentItem> PushIdentMap;

class DBSpool;
class DBHandler
{
public:
    static DBHandler* GetInstance();

protected:
    DBHandler();
    virtual ~DBHandler();

public:
    bool InitDB(const DB_CONF &dbConfig);
    bool Restart();

    // PushMessage operation
    bool InsertPushMessage(const char *tokenid, const char *message);
    PushMessageDBList GetPushMessageWithTokenId(const char *tokenid);
    bool RemovePushMessage(int id);

    // PushIdentification operation
    bool InsertPushIdentification(const char *appid, const char *appkey);
    TPushIdentItem GetAppInfoWithAppid(const char *appid);
    bool RemovePushIdentification(const char *appid);

    // ClientInfo operation
    bool ReplaceClientInfo(const TClientInfoItem &item);

    //ClientOnlineRecord operation
    bool InsertClientOnlineRecord(const TOnlineRecordItem &item);

private:
    inline bool LoadPushIdentificationTable();

private:
    PushIdentMap    m_mapPushIdent;
    DBSpool *m_pDBSpool;
};

#endif // DBHANDLER_H
