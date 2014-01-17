/*
 * Date         : 2010-07-05
 * Author       : Keqin Su
 * File         : main.cpp
 * Description  : Version Server main
 */

#include <signal.h>
#include "DrVerData.hpp"
#include "ConfFile.hpp"
#include "InsideTcpServer.hpp"
#include "DBHandler.hpp"
#include "PushManager.hpp"
#include "ClientManager.hpp"
#include <assert.h>

#define DRVER   "v1.1"
#define MaxMsgDataCount 10000

//global member, extern
SYS_CONF g_SysConf;
//global for Ctrl+C, extern
static bool g_bExit = false;
//tcp & inside server
TcpServer* g_pTcpServer = NULL;
InsideTcpServer* g_pInsideTcpServer = NULL;
//message list
MsgListManager* g_MsgListRecv = NULL;
MsgListManager* g_MsgListData = NULL;
MsgListManager* g_MsgListSnd = NULL;
MsgListManager* g_MsgListIds = NULL;
MsgListManager* g_MsgListWait = NULL;
//count
int g_iRecvCount = 0;
int g_iSendCount = 0;

bool init_config(string strCf);
void _exit_signal(int sig_number);
int MkDir(const char* pDir);
void Count();

int main(int argc, char* argv[]) {

    signal(SIGINT, _exit_signal);
    signal(SIGTERM, _exit_signal);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    //check command parameter
    if (argc < 2) {
        printf("Dr.COM Push Server %s\n\nUsage: drver FILE [-X]\n", DRVER);
        return 1;
    }

    printf("Dr.COM Push Server %s\n", DRVER);

    //init config setting
    string strCf = argv[1];
    if (!init_config(strCf)) {
        printf("Push Server : load config file error, application abort!");
        return 1;
    }

    //init 10k MESSAGE_DATA buffer
    InitMsgList(MaxMsgDataCount);
    //SetFlushBuffer(BUFFER_SIZE_1K * BUFFER_SIZE_1K);
    gvLog(LOG_ERR_USER, "Create global memory done");

    //log mode
    if (argc == 3) {
        strCf = argv[2];
        if (strcmp(strCf.c_str(), "-X") == 0) {
            g_iLogLevel = LOG_STAT;
            SetFlushBuffer(0);
            gvLog(LOG_OFF, "DrVerMain : change to full log mode [change log level to 5th].");
        }
    }

    gvLog(LOG_MSG, "BASE [BINDADD] %s", g_SysConf.m_BaseConf.m_strBindAdd.c_str());
    gvLog(LOG_MSG, "BASE [PORT] %d", g_SysConf.m_BaseConf.m_usPort);
    gvLog(LOG_MSG, "BASE [MAXCONNECT] %d", g_SysConf.m_BaseConf.m_uiMaxConnect);
    gvLog(LOG_MSG, "BASE [PROCPOOL] %d", g_SysConf.m_BaseConf.m_usProcPool);
    gvLog(LOG_MSG, "BASE [SNDPOOL] %d", g_SysConf.m_BaseConf.m_usSndPool);
    gvLog(LOG_MSG, "BASE [RECVPOOL] %d", g_SysConf.m_BaseConf.m_usRecvPool);
    gvLog(LOG_MSG, "BASE [WAITSNDPOOL] %d", g_SysConf.m_BaseConf.m_usWaitSndPool);

    gvLog(LOG_MSG, "INSIDE [BINDADD] %s", g_SysConf.m_InsideConf.m_strBindAdd.c_str());
    gvLog(LOG_MSG, "INSIDE [PORT] %d", g_SysConf.m_InsideConf.m_usPort);

    gvLog(LOG_MSG, "DB [DBSERVER] %s", g_SysConf.m_DBConf.m_strServer.c_str());
    gvLog(LOG_MSG, "DB [DBUSER] %s", g_SysConf.m_DBConf.m_strUserName.c_str());
    gvLog(LOG_MSG, "DB [DBPASS] %s", g_SysConf.m_DBConf.m_strPass.c_str());
    gvLog(LOG_MSG, "DB [DBNAME] %s", g_SysConf.m_DBConf.m_strDbName.c_str());
    gvLog(LOG_MSG, "DB [PORT] %d", g_SysConf.m_DBConf.m_usPort);
    gvLog(LOG_MSG, "DB [CONPOOL] %d", g_SysConf.m_DBConf.m_usThreadPool);

    gvLog(LOG_MSG, "LEAK [100004] %s", g_SysConf.m_Leak.m_b100004 ? "TRUE" : "FALSE");

    gvLog(LOG_MSG, "SYSTEM [LOGLEVEL] %d", g_iLogLevel);
    gvLog(LOG_MSG, "SYSTEM [TIMECHK] %s", g_SysConf.bTimeChk ? "TRUE" : "FALSE");
    gvLog(LOG_MSG, "SYSTEM [NOVERISNEWEST] %s", g_SysConf.bNoVerIsNewest ? "TRUE" : "FALSE");
    gvLog(LOG_MSG, "SYSTEM [LOGPATH] %s", g_SysConf.strLogPath.c_str());
    gvLog(LOG_MSG, "SYSTEM [TEMPPATH] %s", g_SysConf.strTempPath.c_str());

    if (!DBHandler::GetInstance()->InitDB(g_SysConf.m_DBConf)){
        gvLog(LOG_ERR_SYS, "DB initial error");
        return 1;
    }

    g_MsgListRecv = new MsgListManager();
    g_MsgListRecv->Init(GetIdleList());

    g_MsgListData = new MsgListManager();
    g_MsgListData->Init(GetIdleList());

    g_MsgListSnd = new MsgListManager();
    g_MsgListSnd->Init(GetIdleList());

    g_MsgListIds = new MsgListManager();
    g_MsgListIds->Init(GetIdleList());

    g_MsgListWait = new MsgListManager();
    g_MsgListWait->Init(GetIdleList());

    g_pTcpServer = new TcpServer();
    if (!g_pTcpServer->Start()) {
        gvLog(LOG_ERR_SYS, "TcpServer Start fail!");
        return 1;
    }

    g_pInsideTcpServer = new InsideTcpServer();
    if (!g_pInsideTcpServer->InitTcpServer(g_pTcpServer)) {
        gvLog(LOG_ERR_SYS, "InsideTcpServer InitTcpServer() fail!");
        return 1;
    }
    if (!g_pInsideTcpServer->Start()) {
        gvLog(LOG_ERR_SYS, "InsideTcpServer Start fail!");
        return 1;
    }

    Count();

    gvLog(LOG_ERR_SYS, "exit");
    return 0;
}

bool init_config(string strCf)
{
    ConfFile* pCf = new ConfFile();
    pCf->InitConfFile(strCf.c_str(), "");
    if (!pCf->LoadConfFile()) {
        delete pCf;
        return false;
    }

    g_SysConf.m_BaseConf.m_strBindAdd = pCf->GetPrivate("BASE", "BINDADD", "0.0.0.0");
    g_SysConf.m_BaseConf.m_usPort = (short)atoi(pCf->GetPrivate("BASE", "PORT", "80").c_str());
    g_SysConf.m_BaseConf.m_uiMaxConnect = (unsigned)atoi(pCf->GetPrivate("BASE", "MAXCONNECT", "102400").c_str());
    g_SysConf.m_BaseConf.m_usProcPool = (short)atoi(pCf->GetPrivate("BASE", "PROCPOOL", "20").c_str());
    g_SysConf.m_BaseConf.m_usSndPool = (short)atoi(pCf->GetPrivate("BASE", "SNDPOOL", "10").c_str());
    g_SysConf.m_BaseConf.m_usRecvPool = (short)atoi(pCf->GetPrivate("BASE", "RECVPOOL", "10").c_str());
    g_SysConf.m_BaseConf.m_usWaitSndPool = (short)atoi(pCf->GetPrivate("BASE", "WAITSNDPOOL", "10").c_str());

    g_SysConf.m_InsideConf.m_strBindAdd = pCf->GetPrivate("INSIDE", "BINDADD", "192.168.0.1");
    g_SysConf.m_InsideConf.m_usPort = (short)atoi(pCf->GetPrivate("INSIDE", "PORT", "8881").c_str());

    g_SysConf.m_DBConf.m_strServer = pCf->GetPrivate("DB", "DBSERVER", "localhost");
    g_SysConf.m_DBConf.m_strUserName = pCf->GetPrivate("DB", "DBUSER", "root");
    g_SysConf.m_DBConf.m_strPass = pCf->GetPrivate("DB", "DBPASS", "admin");
    g_SysConf.m_DBConf.m_strDbName = pCf->GetPrivate("DB", "DBNAME", "drcom");
    g_SysConf.m_DBConf.m_usPort = (short)atoi(pCf->GetPrivate("DB", "PORT", "80").c_str());
    g_SysConf.m_DBConf.m_usThreadPool = (short)atoi(pCf->GetPrivate("DB", "CONPOOL", "5").c_str());
    if (g_SysConf.m_DBConf.m_usThreadPool < 5) {
        g_SysConf.m_DBConf.m_usThreadPool = 5;
    }

    g_SysConf.m_Leak.m_b100004 = (pCf->GetPrivate("LEAK", "100004", "0") == "0") ? false : true;

    g_iLogLevel = (short)atol(pCf->GetPrivate("SYSTEM", "LOGLEVEL", "0").c_str());
    g_SysConf.bTimeChk = (pCf->GetPrivate("SYSTEM", "TIMECHK", "1") == "1") ? true : false;
    g_SysConf.bNoVerIsNewest = (pCf->GetPrivate("SYSTEM", "NOVERISNEWEST", "0") == "1") ? true : false;
    g_SysConf.strLogPath = pCf->GetPrivate("SYSTEM", "LOGPATH", "/var/DrVerServer/log/");
    g_SysConf.strTempPath = pCf->GetPrivate("SYSTEM", "TEMPPATH", "/var/DrVerServer/temp/");

    MkDir(g_SysConf.strLogPath.c_str());
    MkDir(g_SysConf.strTempPath.c_str());

    delete pCf;
    return true;
}

int MkDir(const char* pDir)
{
    int ret = 0;
    struct stat dirBuf;
    char cDir[BUFFER_SIZE_1K] = {0};
    char cTempDir[BUFFER_SIZE_1K] = {0};

    strcpy(cDir, pDir);
    if (pDir[strlen(pDir)-1] != '/') {
        cDir[strlen(pDir)] = '/';
    }
    int iLen = strlen(cDir);
    for (int i = 0; i < iLen; i++) {
        if ('/' == cDir[i]) {
            if (0 == i) {
                strncpy(cTempDir, cDir, i+1);
                cTempDir[i+1] = '\0';
            } else {
                strncpy(cTempDir, cDir, i);
                cTempDir[i] = '\0';
            }
            ret = stat(cTempDir, &dirBuf);
            if (-1 == ret &&  ENOENT == errno) {
                ret = mkdir(cTempDir, S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    return 0;
                }
                chmod(cTempDir, S_IRWXU | S_IRWXG | S_IRWXO);
            } else if (-1 == ret) {
                return 0;
            }
            if (!(S_IFDIR & dirBuf.st_mode)) {
                return 0;
            }
        }
    }
    return 1;
}

void _exit_signal(int sig_number)
{
    if (!g_bExit){
        gvLog(LOG_OFF, "DrVerMain : Version Server going to shutdow!");
        g_bExit = !g_bExit;

        if (g_pInsideTcpServer) {
            g_pInsideTcpServer->Stop();
            delete g_pInsideTcpServer;
            g_pInsideTcpServer = NULL;
        }
        if (g_pTcpServer) {
            g_pTcpServer->Stop();
            delete g_pTcpServer;
            g_pTcpServer = NULL;
        }
        gvLog(LOG_ERR_USER, "Releasing global memory...");

        delete g_MsgListRecv;
        delete g_MsgListData;
        delete g_MsgListSnd;
        delete g_MsgListIds;
        delete g_MsgListWait;

        DestoryMsgList();
        gvLog(LOG_ERR_USER, "Release global memory done");
        gvLog(LOG_OFF, "DrVerMain : Version Server shutdow!");
        FlushMem2File();
    }
}

void Count()
{
    int iCount = 1, iConnectCount = 0, iMsgIdle = 0, iMsgData = 0, iMsgSnd = 0, iMsgRecv = 0;
    int iDevice = 0;
    int iRecvCount, iSendCount;
    int iMsgDataLimitCount = (int)(MaxMsgDataCount * 1.5);

    while(!g_bExit) {
        sleep(1);
        if ((iCount % 60) != 0) {
            iCount++;
            if ((iCount % 5) == 0) {
                FlushMem2File();
            }
        } else {
            iCount = 1;
            iConnectCount = g_pTcpServer->GetConnect();
            iDevice = PushManager::GetInstance()->GetOnlineDeviceCout();
            g_pTcpServer->ReSetConnect();
            iMsgIdle = GetIdleMsgBuffCount();
            iMsgData = g_MsgListData->GetMsgListBuffCount();
            iMsgSnd = g_MsgListSnd->GetMsgListBuffCount();
            iMsgRecv = g_MsgListRecv->GetMsgListBuffCount();
            iRecvCount = g_iRecvCount;
            g_iRecvCount = 0;
            iSendCount = g_iSendCount;
            g_iSendCount = 0;
            if (iConnectCount > 0) {
                gvLog(LOG_ERR_USER, "there are %d connections, recv packet %d, send packet %d in last 60 seconds, %d devices online, idle remains %d items, parser remains %d items, send remain %d items, recv remain %d items.",
                                    iConnectCount, iRecvCount, iSendCount, iDevice, iMsgIdle, iMsgData, iMsgSnd, iMsgRecv);
            }

            if (iMsgIdle > iMsgDataLimitCount) {
                gvLog(LOG_ERR_USER, "MsgBuff is %d now, more than %d, close PushServer", iMsgIdle, iMsgDataLimitCount);
                FlushMem2File();
                assert(false);
                exit(0);
            }

            // 统计入库
            if (!ClientManager::GetInstance()->StorageClientInfoMap()) {
                gvLog(LOG_ERR_USER, "ClientManager::GetInstance()->StorageClientInfoMap() ERR: fail!");
            }

            if (!ClientManager::GetInstance()->StorageOnlineRecordList()) {
                gvLog(LOG_ERR_USER, "ClientManager::GetInstance()->StorageOnlineRecordList() ERR: fail!");
            }
        }
    }
}
