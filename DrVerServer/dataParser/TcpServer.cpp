/*
 * File         : TCPServer.cpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server
 */

#include "TcpServer.hpp"
#include "MessageMgr.hpp"
#include "PushHandler.hpp"
#include "DBSpool.hpp"
#include "MatchPacketHandler.hpp"
#include "PushManager.hpp"
#include "DBHandler.hpp"
#include <linux/sockios.h>

extern SYS_CONF g_SysConf;

extern MsgListManager* g_MsgListRecv;
extern MsgListManager* g_MsgListData;
extern MsgListManager* g_MsgListSnd;
extern MsgListManager* g_MsgListWait;

#define WAITTIME_INVAL  3000   /// second

TcpServer::TcpServer()
{
    m_bStart = false;
    m_bReStart = false;
    m_iConnect = 0;
    m_c2s = NULL;
    m_pDBSpool = NULL;
    m_pProcThread = NULL;
    m_pSndThread = NULL;
    m_pRecvThread = NULL;
    m_pMainThread = NULL;
    m_pWaitThread = NULL;
//    pthread_mutex_init(&m_tMutex, NULL);
    pthread_mutex_init(&m_tMutexConnect, NULL);
    pthread_mutex_init(&m_tMutexMio, NULL);
}

TcpServer::~TcpServer()
{
    Stop();
    pthread_mutex_destroy(&m_tMutexConnect);
//	pthread_mutex_destroy(&m_tMutex);
	pthread_mutex_destroy(&m_tMutexMio);
	DestoryMsgList();
}

bool TcpServer::Start()
{
    if (m_bStart){
        return true;
    }

    Stop();

    //create server object
    m_c2s = (c2s_t) malloc(sizeof(struct c2s_st));
    memset(m_c2s, 0, sizeof(struct c2s_st));
    m_c2s->server_fd = 0;
    m_c2s->local_ip = strdup(g_SysConf.m_BaseConf.m_strBindAdd.c_str());
    m_c2s->local_port = g_SysConf.m_BaseConf.m_usPort;
    m_c2s->io_max_fds = g_SysConf.m_BaseConf.m_uiMaxConnect;
    m_c2s->pvoid = (void*)this;
    /*
    m_c2s->mio = mio_new(m_c2s->io_max_fds);
    if (!m_c2s->mio) {
    */
    m_c2s->mio2 = mio2_new(m_c2s->io_max_fds);
    if (!m_c2s->mio2) {
        gvLog(LOG_ERR_SYS, "(TcpServer::Start) Version Server Create Failed while creating mio!");
        return false;
    }

    //create DB pool
    m_pDBSpool = new DBSpool();
    if (m_pDBSpool == NULL) {
        gvLog(LOG_ERR_SYS, "(TcpServer::Start) Version Server Create Failed while creating DB spool!");
        return false;
    }
    if (!m_pDBSpool->SetConnection(g_SysConf.m_DBConf.m_usThreadPool)) {
        gvLog(LOG_ERR_SYS, "(TcpServer::Start) Version Server Create Failed while setting DB spool connection!");
        return false;
    }
    m_pDBSpool->SetDBparm(g_SysConf.m_DBConf.m_strServer, g_SysConf.m_DBConf.m_usPort, g_SysConf.m_DBConf.m_strDbName,
                            g_SysConf.m_DBConf.m_strUserName, g_SysConf.m_DBConf.m_strPass);
    if (!m_pDBSpool->Connect()) {
        gvLog(LOG_ERR_SYS, "(TcpServer::Start) Core Server Create Failed while connecting to database!");
        return false;
    }
    gvLog(LOG_OFF, "(TcpServer::Start) Init DB(ip=%s, port=%d) done.", g_SysConf.m_DBConf.m_strServer.c_str(), g_SysConf.m_DBConf.m_usPort);

    //init DBMapping
    {
//        CAutoLock lock(&m_tMutex);
//        if (m_VerDBMapping.InitByDBSpool(m_pDBSpool) == -1){
//            gvLog(LOG_ERR_SYS, "(TcpServer::Start) Core Server Create Failed while create DB mapping!");
//            return false;
//        }
    }
    gvLog(LOG_OFF, "(TcpServer::Start) Init mapping done.");

    //create server
    /*
    m_c2s->server_fd = mio_listen(m_c2s->mio, m_c2s->local_port, m_c2s->local_ip, _c2s_client_mio_callback, (void*)m_c2s);
    */
    m_c2s->server_fd = mio2_listen(m_c2s->mio2, m_c2s->local_port, m_c2s->local_ip, _c2s_client_mio2_callback, (void*)m_c2s);
    if (m_c2s->server_fd < 0) {
        gvLog(LOG_ERR_SYS, "(TcpServer::Start) Version Server Create Failed while creating socket server!");
        return false;
    }

    m_bStart = true;
    m_bReStart = false;
    ReSetConnect();
    //create proc pool
    m_pProcThread = new pthread_t[g_SysConf.m_BaseConf.m_usProcPool];
    for (int i = 0; i < g_SysConf.m_BaseConf.m_usProcPool; i++) {
        pthread_create(&(m_pProcThread[i]), NULL, &DataParse, (void*)m_c2s);
    }

    //create send pool
    m_pSndThread = new pthread_t[g_SysConf.m_BaseConf.m_usSndPool];
    for (int i = 0; i < g_SysConf.m_BaseConf.m_usSndPool; i++) {
        pthread_create(&(m_pSndThread[i]), NULL, &RspParse, (void*)m_c2s);
    }

    //create receive pool
    m_pRecvThread = new pthread_t[g_SysConf.m_BaseConf.m_usRecvPool];
    for (int i = 0; i < g_SysConf.m_BaseConf.m_usRecvPool; i++) {
        pthread_create(&(m_pRecvThread[i]), NULL, &RecvParse, (void*)m_c2s);
    }

    m_pMainThread = new pthread_t;
    pthread_create(m_pMainThread, NULL, &MioRun, (void*)m_c2s);

    m_pWaitThread = new pthread_t[g_SysConf.m_BaseConf.m_usWaitSndPool];
    for (int i = 0; i < g_SysConf.m_BaseConf.m_usRecvPool; i++) {
        pthread_create(&(m_pWaitThread[i]), NULL, &WaitHandle, (void*)m_c2s);
    }

    gvLog(LOG_OFF, "(TcpServer::Start) Version Server runing (ip=%s, port=%d)...", m_c2s->local_ip, m_c2s->local_port);

    return true;
}

bool TcpServer::Stop()
{
    m_bStart = false;
    bool bStop = false;

    //wait for mio run
    if (m_pMainThread) {
        pthread_join(*m_pMainThread, NULL);
        delete m_pMainThread;
        m_pMainThread = NULL;
    }

    //stop server
	if (m_c2s) {
        //if (m_c2s->mio) {
        //    mio_free(m_c2s->mio);
        //}
        if (m_c2s->mio2) {
            mio2_free(m_c2s->mio2);
        }
        free(m_c2s);
        m_c2s = NULL;
        bStop = true;
    }

    //wait for proc pool
    if (m_pProcThread) {
        for (int i = 0; i < g_SysConf.m_BaseConf.m_usProcPool; i++) {
            pthread_join(m_pProcThread[i], NULL);
        }
        delete[] m_pProcThread;
        m_pProcThread = NULL;
    }

    //wait for send pool
    if (m_pSndThread) {
        for (int i = 0; i < g_SysConf.m_BaseConf.m_usSndPool; i++) {
            pthread_join(m_pSndThread[i], NULL);
        }
        delete[] m_pSndThread;
        m_pSndThread = NULL;
    }

    //wait for recv pool
    if (m_pRecvThread) {
        for (int i = 0; i < g_SysConf.m_BaseConf.m_usRecvPool; i++) {
            pthread_join(m_pRecvThread[i], NULL);
        }
        delete[] m_pRecvThread;
        m_pRecvThread = NULL;
    }

    //wait
    if (m_pWaitThread) {
        for (int i = 0; i < g_SysConf.m_BaseConf.m_usWaitSndPool; i++) {
            pthread_join(m_pWaitThread[i], NULL);
        }
        delete[] m_pWaitThread;
        m_pWaitThread = NULL;
    }

    //reset data & rsp & recv buffer
    g_MsgListData->ResMsg();
    g_MsgListSnd->ResMsg();
    g_MsgListRecv->ResMsg();
    g_MsgListWait->ResMsg();

    //release DB mapping
    {
//        CAutoLock lock(&m_tMutex);
//        m_VerDBMapping.UnInit();
    }

    //release DB pool
    if (m_pDBSpool) {
        delete m_pDBSpool;
        m_pDBSpool = NULL;
    }

    if (bStop) {
        gvLog(LOG_OFF, "(TcpServer::Stop) Version Server stop!");
    }
    return true;
}

bool TcpServer::ReStart()
{
    if (!m_c2s) return false;

    m_bReStart = true;
    gvLog(LOG_OFF, "(TcpServer::ReStart) Version Server restarting...");

    //wait for proc pool
    if (m_pProcThread) {
        for (int i = 0; i < g_SysConf.m_BaseConf.m_usProcPool; i++) {
            pthread_join(m_pProcThread[i], NULL);
        }
        delete[] m_pProcThread;
        m_pProcThread = NULL;
    }
    gvLog(LOG_OFF, "(TcpServer::ReStart) stop process thread ok");

    //ignore all the data packet in DataMsg
    LPMESSAGE_DATA pMsg = g_MsgListData->Get_Msg();
    while (pMsg) {
        mio2_close(m_c2s->mio2, pMsg->iFd);
        PutIdleMsgBuff(pMsg);
        pMsg = g_MsgListData->Get_Msg();
        usleep(1);
    }
    gvLog(LOG_OFF, "(TcpServer::ReStart) close data buffer ok");

    //ignore all the recv packet in RecvMsg
    pMsg = g_MsgListRecv->Get_Msg();
    while (pMsg) {
        mio2_close(m_c2s->mio2, pMsg->iFd);
        PutIdleMsgBuff(pMsg);
        pMsg = g_MsgListRecv->Get_Msg();
        usleep(1);
    }
    gvLog(LOG_OFF, "(TcpServer::ReStart) close recv buffer ok");

    //restore DB mapping
    {
//        CAutoLock lock(&m_tMutex);
//        m_VerDBMapping.UnInit();
//        if (m_VerDBMapping.InitByDBSpool(m_pDBSpool) == -1){
//            gvLog(LOG_ERR_SYS, "(TcpServer::ReStart) restore db mapping fail!");
//        } else {
//            gvLog(LOG_ERR_SYS, "(TcpServer::ReStart) restore db mapping ok");
//        }
        DBHandler::GetInstance()->Restart();
    }

    m_bReStart = false;
    m_pProcThread = new pthread_t[g_SysConf.m_BaseConf.m_usProcPool];
    for (int i = 0; i < g_SysConf.m_BaseConf.m_usProcPool; i++) {
        pthread_create(&(m_pProcThread[i]), NULL, &DataParse, (void*)m_c2s);
    }

    gvLog(LOG_OFF, "(TcpServer::ReStart) Version Server restart finish.");
    return true;
}

bool TcpServer::IsRunning()
{
    return m_bStart;
}

bool TcpServer::IsRestarting()
{
    return m_bReStart;
}

int TcpServer::AddConnect()
{
    CAutoLock lock(&m_tMutexConnect);
    return m_iConnect++;
}

void TcpServer::ReSetConnect()
{
    CAutoLock lock(&m_tMutexConnect);
    m_iConnect = 0;
}

int TcpServer::GetConnect()
{
    return m_iConnect;
}

void* TcpServer::DataParse(void* pData)
{
    TcpServer* pthis = NULL;
    mio2_t m = NULL;
    LPMESSAGE_DATA pMsg = NULL;
    DataParser* pDataParser = NULL;

    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (TcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(TcpServer::DataParse) process pool start fail by error param");
        return NULL;
    }

    pDataParser = new DataParser();
    if (!pDataParser) goto ErrDataParse;

    while (pthis->IsRunning() && !pthis->IsRestarting()) {
        pMsg = g_MsgListData->Get_Msg();
        if (pMsg) {
            gvLog(LOG_MSG, "(TcpServer::DataParse) entering data parser...");
            pDataParser->ParserData(pMsg);
            gvLog(LOG_MSG, "(TcpServer::DataParse) left data parser...");
            g_MsgListSnd->Put_Msg(pMsg);
        }
        usleep(1);
    }

ErrDataParse:
//    if (pVerDBMapping) {
//        pVerDBMapping->UnInit();
//        delete pVerDBMapping;
//    }
    if (pDataParser) {
        delete pDataParser;
    }
}

void* TcpServer::RspParse(void* pData)
{
    TcpServer* pthis = NULL;
    mio2_t m = NULL;
    LPMESSAGE_DATA pMsg = NULL;

    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (TcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(TcpServer::RspParse) response pool start fail by error param");
        return NULL;
    }

    RspParser* pRspParser = new RspParser();
    if (!pRspParser) return NULL;

    int iSendResult = 0;
    while (pthis->IsRunning()) {
        pMsg = g_MsgListSnd->Get_Msg();
        if (pMsg) {
            gvLog(LOG_MSG, "(TcpServer::RspParse) entering send parser...  pMsg:%p pMsg->iFd:%d", pMsg, pMsg->iFd);
            iSendResult = pRspParser->ParserData(pMsg);
            gvLog(LOG_MSG, "(TcpServer::RspParse) iSendResult:%d pMsg->iFd:%d", iSendResult, pMsg->iFd);
            if (0 == iSendResult) {
                // time out
                //send buffer reget
                g_MsgListSnd->Put_Msg(pMsg);
            }
            else if (-1 == iSendResult){
                // fail
                /*
                {
                    CAutoLock lock(&(pthis->m_tMutexMio));
                    mio_close_quick(m, pMsg->iFd);
                }
                */
                if (IsAvailableIndex(pMsg->iFd)) {
                    PushManager::GetInstance()->SendFail(pMsg);
                    mio2_close(m, pMsg->iFd);
                }
                else {
                    /// ERROR 不应该有这种情况出现
                    gvLog(LOG_ERR_USER, "(TcpServer::RspParse) ERR: pMsg->iFd:%d", pMsg->iFd);
                }

                gvLog(LOG_MSG, "(TcpServer::RspParse) MSG: fail iFd:%d pMsg:%p", pMsg->iFd, pMsg);
                PutIdleMsgBuff(pMsg);
            }
            else if (-2 == iSendResult) {
                // wait

            }
            else {
                // success
                gvLog(LOG_MSG, "(TcpServer::RspParse) iFd:%d pMsg->bSendAndClose:%d", pMsg->iFd, pMsg->bSendAndClose);
                if (pMsg->bSendAndClose){
                    mio2_close(m, pMsg->iFd);
                }
                else {
                    PushManager::GetInstance()->SendSuccess(pMsg->iFd);
                }

                gvLog(LOG_MSG, "(TcpServer::RspParse) MSG: success iFd:%d pMsg:%p", pMsg->iFd, pMsg);
                PutIdleMsgBuff(pMsg);
            }
            gvLog(LOG_MSG, "(TcpServer::RspParse) left send parser...");
        }
        usleep(1);
    }
}

void* TcpServer::RecvParse(void* pData)
{
    TcpServer* pthis = NULL;
    mio2_t m = NULL;
    LPMESSAGE_DATA pMsg = NULL;
    int iLen = 0;

    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (TcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(TcpServer::RecvParse) receive pool start fail by error param");
        return NULL;
    }

    RecvParser* pRecvParser = new RecvParser();
    if (!pRecvParser) return NULL;

    LPMESSAGE_DATA pErrMsgData = NULL;
    while (pthis->IsRunning()) {
        pMsg = g_MsgListRecv->Get_Msg();
        if (pMsg) {
            gvLog(LOG_MSG, "(TcpServer::RecvParse) entering receive parser... iFd:%d", pMsg->iFd);
            iLen = pRecvParser->ParserData(pMsg);
            gvLog(LOG_MSG, "(TcpServer::RecvParse) MSG: iLen:%d iFd:%d", iLen, pMsg->iFd);
            if (iLen > 0) {
                //put into data parser
                pthis->AddConnect();

                pMsg = MatchPacketHandler::GetInstance()->Handling(pMsg, pErrMsgData);
                if (NULL != pMsg){
                    g_MsgListData->Put_Msg(pMsg);
                }
                else if (NULL != pErrMsgData){
                    pErrMsgData->bSendAndClose = true;
                    g_MsgListSnd->Put_Msg(pErrMsgData);
                }
                else {
                    if (NULL != pMsg) {
                        gvLog(LOG_MSG, "(TcpServer::RecvParse) MSG: MatchPacketHandler() iFd:%d", pMsg->iFd);
                    }
                }
            } else {
                if (iLen == 0) {
                    //maybe (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN)
                    gvLog(LOG_MSG, "(TcpServer::RecvParse) (iLen == 0) iFd:%d", pMsg->iFd);
                } else {
                    gvLog(LOG_MSG, "(TcpServer::RecvParse) mio2_close iFd:%d", pMsg->iFd);
                    mio2_close(m, pMsg->iFd);
                }
                gvLog(LOG_MSG, "(TcpServer::RecvParse) MSG: fail iFd:%d pMsg:%p", pMsg->iFd, pMsg);
                PutIdleMsgBuff(pMsg);
            }
            gvLog(LOG_MSG, "(TcpServer::RecvParse) left receive parser... pMsg:%p", pMsg);
        }
        usleep(1);
    }
}

void* TcpServer::MioRun(void *pData)
{
    TcpServer* pthis = NULL;
    mio2_t m = NULL;
    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (TcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(TcpServer::MioRun) mio run start fail by error param");
        return NULL;
    }

    while (pthis->IsRunning()) {
        {
            CAutoLock lock(&(pthis->m_tMutexMio));
            mio2_run(m, 5);
        }
        usleep(1);
    }
    return NULL;
}

void* TcpServer::WaitHandle(void* pData)
{
    TcpServer* pthis = NULL;
    mio2_t m = NULL;
    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (TcpServer*)(((c2s_t)pData)->pvoid);
    }

    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(TcpServer::WaitHandle) wait pool start fail by error param");
        return NULL;
    }

    LPMESSAGE_DATA pMsg = NULL;
    size_t ileftUnsend;
    int iSetp = 0;
    bool bIsError = true;
    unsigned int iTimeOut = 0;
    while (pthis->IsRunning()) {
        pMsg = g_MsgListWait->Get_Msg();
        if (pMsg) {
            bIsError = true;
            if (ioctl(pMsg->iFd, SIOCOUTQ, (char*)&ileftUnsend) >= 0) {
                if (0 == ileftUnsend) {
                    /// success
                    gvLog(LOG_MSG, "(TcpServer::WaitHandle) MSG: success iFd:%d pMsg:%p tokenid:%s msg:%s", pMsg->iFd, pMsg, pMsg->cBuffer, pMsg->pData->m_cRsp);
                    if (pMsg->bSendAndClose){
                        mio2_close(m, pMsg->iFd);
                    }
                    else {
                        PushManager::GetInstance()->SendSuccess(pMsg->iFd);
                    }
                    gvLog(LOG_MSG, "(TcpServer::WaitHandle) MSG: handle finish iFd:%d pMsg:%p tokenid:%s", pMsg->iFd, pMsg, pMsg->cBuffer);
                    pMsg->pData->m_cRsp[pMsg->pData->m_usRspTotal] = 0;
                    PutIdleMsgBuff(pMsg);
                    bIsError = false;
                }
                else {
                    iTimeOut = pMsg->lLastSendTime + WAITTIME_INVAL;
                    if (GetTickCount() < iTimeOut) {
                        /// wait
                        g_MsgListWait->Put_Msg(pMsg);
    //                    gvLog(LOG_MSG, "(TcpServer::WaitHandle) MSG: GetTickCount:%d pMsg->lLastSendTime:%d", GetTickCount(), pMsg->lLastSendTime);
                        bIsError = false;
                    }
                }
            }

            if (bIsError) {
                /// fail
                PushManager::GetInstance()->SendFail(pMsg);
                mio2_close(m, pMsg->iFd);

                gvLog(LOG_MSG, "(TcpServer::WaitHandle) MSG: fail iFd:%d pMsg:%p", pMsg->iFd, pMsg);
                PutIdleMsgBuff(pMsg);
            }
        }
        usleep(1);
    }
    return NULL;
}

/*
int TcpServer::_c2s_client_mio_callback(mio_t m, mio_action_t a, int fd, void *data, void *arg)
*/
int TcpServer::_c2s_client_mio2_callback(mio2_t m, mio2_action_t a, int fd, void *data, void *arg)
{
    TcpServer* pthis = NULL;
    c2s_t c2s = NULL;
    if (arg) {
        c2s = (c2s_t)arg;
        pthis = (TcpServer*)(((c2s_t)arg)->pvoid);
    }

    switch(a) {
        /*
        case action_READ:
        */
        case mio2_action_READ:
            //do NOT accept new connect while restart
            if (pthis->IsRestarting()) {
				return 0;
            }
            return pthis->RevData(fd, m);
        /*
        case action_WRITE:
        */
        case mio2_action_WRITE:
            /*
            return 0;
            */
            return 1;
        /*
        case action_CLOSE:
        */
        case mio2_action_CLOSE:
            return 0;
        /*
        case action_ACCEPT:
        */
        case mio2_action_ACCEPT:
            //c2s->highfd = GetHighfd(m);
            gvLog(LOG_MSG, "(TcpServer::_c2s_client_mio_callback) Accept connect IP=%s(%d)...", data, fd);
            //add by using epoll
            return 1;
    }
    return 0;
}

/*
int TcpServer::RevData(int iFd, mio_t m)
*/
int TcpServer::RevData(int iFd, mio2_t m)
{
    int iMsgCount = g_MsgListData->GetMsgListBuffCount();
    gvLog(LOG_MSG, "(TcpServer::RevData) iFd:%d Message list remains %d items, idle list remains %d items.", iFd, iMsgCount, GetIdleMsgBuffCount());
    gvLog(LOG_MSG, "(TcpServer::RevData) iFd:%d g_MsgListRecv:%d g_MsgListData:%d g_MsgListSnd:%d g_MsgListWait:%d"
          , iFd
          , g_MsgListRecv->GetMsgListBuffCount()
          , g_MsgListData->GetMsgListBuffCount()
          , g_MsgListSnd->GetMsgListBuffCount()
          , g_MsgListWait->GetMsgListBuffCount());

    if (iMsgCount > GetMsgLimit()) {
        usleep(GET_SOME_REST);
    } else if (iMsgCount > GetInitMsgLimit()) {
        usleep(SIT_BACK);
    }

    LPMESSAGE_DATA pMsg = GetIdleMsgBuff();
    if (!pMsg) {
        return 0;
    }

	if (!pMsg->pData) {
        delete pMsg;
        return 0;
    }

    pMsg->pData->ReSet();
    pMsg->iFd = iFd;
    g_MsgListRecv->Put_Msg(pMsg);
    return 1;
}
