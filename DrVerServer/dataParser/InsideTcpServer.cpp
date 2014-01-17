/*
 * File         : InsideTcpServer.cpp
 * Date         : 2010-07-02
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for simple tcp server
 */

#include "InsideTcpServer.hpp"

extern SYS_CONF g_SysConf;
extern MsgListManager* g_MsgListIds;

#define INSIDE_THREAD_COUNT     1

InsideTcpServer::InsideTcpServer()
{
    m_bStart = false;
    m_pTcpServer = NULL;
    m_c2s = NULL;
    m_pProcThread = NULL;
    m_pMainThread = NULL;
    pthread_mutex_init(&m_tMutex, NULL);
    pthread_mutex_init(&m_tMutexMio, NULL);
}

InsideTcpServer::~InsideTcpServer()
{
    Stop();
	pthread_mutex_destroy(&m_tMutex);
	pthread_mutex_destroy(&m_tMutexMio);
}

bool InsideTcpServer::InitTcpServer(TcpServer* pTcpServer)
{
    if (pTcpServer) {
        m_pTcpServer = pTcpServer;
        return true;
    }
    return false;
}

bool InsideTcpServer::Start()
{
    if (m_bStart){
        return true;
    }

    Stop();

    gvLog(LOG_OFF, "(InsideTcpServer::Start) BINDADD %s", g_SysConf.m_InsideConf.m_strBindAdd.c_str());
    gvLog(LOG_OFF, "(InsideTcpServer::Start) PORT %d", g_SysConf.m_InsideConf.m_usPort);
    gvLog(LOG_OFF, "(InsideTcpServer::Start) MAXCONNECT %d", 1);
    //create server object
    m_c2s = (c2s_t) malloc(sizeof(struct c2s_st));
    memset(m_c2s, 0, sizeof(struct c2s_st));
    m_c2s->server_fd = 0;
    m_c2s->local_ip = strdup(g_SysConf.m_InsideConf.m_strBindAdd.c_str());
    m_c2s->local_port = g_SysConf.m_InsideConf.m_usPort;
    m_c2s->io_max_fds = 1;
    m_c2s->pvoid = (void*)this;
    m_c2s->mio2 = mio2_new(m_c2s->io_max_fds);
    if (!m_c2s->mio2) {
        gvLog(LOG_ERR_SYS, "(InsideTcpServer::Start) Inside Server Create Failed while creating mio2!");
        return false;
    }

    //create server
    m_c2s->server_fd = mio2_listen(m_c2s->mio2, m_c2s->local_port, m_c2s->local_ip, _c2s_client_mio2_callback, (void*)m_c2s);
    if (m_c2s->server_fd < 0) {
        gvLog(LOG_ERR_SYS, "(InsideTcpServer::Start) Inside Server Create Failed while creating socket server!");
        return false;
    }

    m_bStart = true;

    //create proc pool
    m_pProcThread = new pthread_t[INSIDE_THREAD_COUNT];
    for (int i = 0; i < INSIDE_THREAD_COUNT; i++) {
        pthread_create(m_pProcThread + i, NULL, &DataParser, (void*)m_c2s);
    }

    m_pMainThread = new pthread_t;
    pthread_create(m_pMainThread, NULL, &MioRun, (void*)m_c2s);

    return true;
}

bool InsideTcpServer::Stop()
{
    m_bStart = false;
    bool bStop = false;

    //wait for mio2 run
    if (m_pMainThread) {
        pthread_join(*m_pMainThread, NULL);
        delete m_pMainThread;
        m_pMainThread = NULL;
    }

    //wait for proc pool
    if (m_pProcThread) {
        for (int i = 0; i < INSIDE_THREAD_COUNT; i++) {
            pthread_join(m_pProcThread[i], NULL);
        }
        delete[] m_pProcThread;
        m_pProcThread = NULL;
    }

    //stop server
	if (m_c2s) {
	    // free mio2 will crash ????
//        if (m_c2s->mio2) {
//            mio2_free(m_c2s->mio2);
//        }
        free(m_c2s);
        m_c2s = NULL;
        bStop = true;
    }

    if (bStop) {
        gvLog(LOG_OFF, "(InsideTcpServer::Stop) Inside Server stop!");
    }
    return true;
}

bool InsideTcpServer::IsRunning()
{
    return m_bStart;
}

void* InsideTcpServer::DataParser(void* pData)
{
    InsideTcpServer* pthis = NULL;
    mio2_t m = NULL;
    int iProcessResult = 0;
    bool bReStartFlag = false;
    bool bCloseFlag = false;

    gvLog(LOG_MSG, "(InsideTcpServer::DataParser) start");

    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (InsideTcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(InsideTcpServer::RspParser) response pool start fail by error param");
        return NULL;
    }

    gvLog(LOG_MSG, "(InsideTcpServer::DataParser) while");

    LPMESSAGE_DATA pMsg = NULL;
    while (pthis->IsRunning()) {
        bReStartFlag = false;
        bCloseFlag = false;
        pMsg = g_MsgListIds->Get_Msg();
        if (pMsg) {
            iProcessResult = pthis->SimpleHttp(pMsg->pData->m_cRsp);
            if (iProcessResult == 1) {
                snprintf(pMsg->pData->m_cRsp, sizeof(pMsg->pData->m_cRsp), HTTP_REPONSE, HTTP_200_OK, HTTP_TEXT, 0);
                bReStartFlag = true;
                gvLog(LOG_OFF, "(InsideTcpServer::DataParser) begin to reload db, please wait...");
            } else if (iProcessResult == 2) {
                snprintf(pMsg->pData->m_cRsp, sizeof(pMsg->pData->m_cRsp), HTTP_REPONSE, HTTP_200_OK, HTTP_TEXT, 0);
                bCloseFlag = true;
                gvLog(LOG_OFF, "(InsideTcpServer::DataParser) close!");
            } else {
                snprintf(pMsg->pData->m_cRsp, sizeof(pMsg->pData->m_cRsp), HTTP_REPONSE, HTTP_403, HTTP_TEXT, 0);
                gvLog(LOG_OFF, "(InsideTcpServer::DataParser) protocol error!");
            }
            gvLog(LOG_STAT, "(InsideTcpServer::DataParser) SendData:\r\n%s", pMsg->pData->m_cRsp);
            pthis->SendData(pMsg->iFd, pMsg->pData->m_cRsp, strlen(pMsg->pData->m_cRsp));
            {
                CAutoLock lock(&(pthis->m_tMutexMio));
                mio2_close_quick(m, pMsg->iFd);
            }
            PutIdleMsgBuff(pMsg);
            if (bReStartFlag) {
                pthis->m_pTcpServer->ReStart();
                gvLog(LOG_OFF, "(InsideTcpServer::DataParser) reload db finish");
            }
            else if (bCloseFlag) {
                pthis->m_pTcpServer->Stop();
                pthis->Stop();
                gvLog(LOG_OFF, "(InsideTcpServer::DataParser) close finish");
            }
        }
        usleep(1);
    }

    gvLog(LOG_OFF, "(InsideTcpServer::DataParser) exit");
    exit(0);

    return NULL;
}

void* InsideTcpServer::MioRun(void *pData)
{
    InsideTcpServer* pthis = NULL;
    mio2_t m = NULL;
    if (pData) {
        m = ((c2s_t)pData)->mio2;
        pthis = (InsideTcpServer*)(((c2s_t)pData)->pvoid);
    }
    if (!pthis || !m) {
        gvLog(LOG_ERR_SYS, "(InsideTcpServer::MioRun) mio2 run start fail by error param");
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

int InsideTcpServer::_c2s_client_mio2_callback(mio2_t m, mio2_action_t a, int fd, void *data, void *arg)
{
    InsideTcpServer* pthis = NULL;
    c2s_t c2s = NULL;
    if (arg) {
        c2s = (c2s_t)arg;
        pthis = (InsideTcpServer*)(((c2s_t)arg)->pvoid);
    }

    int iBytes=0;

    gvLog(LOG_MSG, "(InsideTcpServer::_c2s_client_mio2_callback) a: %d", a);
    switch(a) {
        case mio2_action_READ:
            gvLog(LOG_MSG, "(InsideTcpServer::_c2s_client_mio2_callback) action_READ");
            ioctl(fd, FIONREAD, &iBytes);
            gvLog(LOG_MSG, "(InsideTcpServer::_c2s_client_mio2_callback) recv:%d...", iBytes);
            if(iBytes == 0) {
                return 0;
            }
            if (pthis->RevData(fd, iBytes, m) < 0) {
                return 0;
            }
            return iBytes;
        case mio2_action_WRITE:
            return 0;
        case mio2_action_CLOSE:
            return 0;
        case mio2_action_ACCEPT:
            //c2s->highfd = GetHighfd(m);
            gvLog(LOG_MSG, "(InsideTcpServer::_c2s_client_mio2_callback) Accept connect IP=%s(%d)...", data, fd);
            return 1;
    }
    return 0;
}

int InsideTcpServer::SimpleHttp(char* pData)
{
    char *p = strstr(pData, END_WITH_LINE);
    p -= 9;//the len of (" http/1.0")

    int iEnd = (int)(p - pData);
    if (iEnd < 0 || iEnd > BUFFER_SIZE_2K){
        return 0;
    }
    *p = '\0';
    for (int i = 0; i <= iEnd; i++){
        if (pData[i] > 96 && pData[i] < 123){
            pData[i] = pData[i] - 32;
        }
    }
    if (pData[0] != 'P' || pData[1] != 'O' || pData[2] != 'S' || pData[3] != 'T'){
        gvLog(LOG_WARNING, "(InsideTcpServer::SimpleHttp) do not match http get request!");
        return 0;
    }

    if (NULL != strstr(pData, HTTP_RELOAD)){
        return 1;
    } else if (NULL != strstr(pData, HTTP_CLOSE)) {
        return 2;
    } else {
        gvLog(LOG_WARNING, "(InsideTcpServer::SimpleHttp) do not match http reload request! pData:\n%s", pData);
    }
    return 0;
}

int InsideTcpServer::RevData(int iFd, int iBytes, mio2_t m)
{
    gvLog(LOG_MSG, "(InsideTcpServer::RevData) start");

    //only parse one request
    if (g_MsgListIds->GetMsgListBuffCount() > 0) {
        gvLog(LOG_ERR_SYS, "(InsideTcpServer::RevData) too many reload request from socket(%d).", iFd);
        usleep(GET_SOME_REST);
        return -1;
    }

    LPMESSAGE_DATA pMsg = GetIdleMsgBuff();
    if (!pMsg) {
        return -1;
    }

	if (!pMsg->pData) {
        delete pMsg;
        return -1;
    }

    int iRev = iBytes, iLen = 0;
    pMsg->pData->ReSet();

    iLen = recv(iFd, pMsg->pData->m_cRsp, iRev, 0);
    if (iLen < 0) {
        if(errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN) {
            PutIdleMsgBuff(pMsg);
			gvLog(LOG_WARNING, "(InsideTcpServer::RevData) errno == EWOULDBLOCK end.");
			return 0;
		}
		gvLog(LOG_WARNING, "(InsideTcpServer::RevData) socket(%d) error end.", iFd);
    } else if (iLen == 0) {
        gvLog(LOG_WARNING, "(InsideTcpServer::RevData) remote socket(%d) close.", iFd);
    } else {
        pMsg->pData->m_cRsp[iLen] = '\0';
        if (pMsg->pData->m_cRsp[iLen - 1] == '\n' && pMsg->pData->m_cRsp[iLen - 2] == '\r'){
            pMsg->iFd = iFd;
            pMsg->sTimes = MAX_SEND_TIMES;
            g_MsgListIds->Put_Msg(pMsg);
            return iLen;
        }
        gvLog(LOG_WARNING, "(InsideTcpServer::RevData) can not get http end data from recv buffer. pMsg->pData->m_cRsp:\r\n", pMsg->pData->m_cRsp);
    }

    PutIdleMsgBuff(pMsg);
    return -1;
}

int InsideTcpServer::SendData(int iFd, char* pData, int iLen)
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
            gvLog(LOG_WARNING, "(InsideTcpServer::SendData) waitting for send data timeout.");
            break;
        default:
            if (fds.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                return -1;
            } else if (fds.revents & POLLOUT) {
                iRet = send(iFd, pData, iLen, 0);
                if (iRet == 0) {
                    return -1;
                }
                if (iRet < 0) {
                    if (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN) {
                        gvLog(LOG_WARNING, "(InsideTcpServer::SendData) errno == EWOULDBLOCK end.");
                        return 0;
                    }
                    return -1;
                }
                iSend += iRet;
            }
            break;
    }
    return iSend;
}
