/*
 * File         : InsideTcpServer.hpp
 * Date         : 2010-07-02
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for simple tcp server
 */

#ifndef _INC_INSIDETCPSERVER_
#define _INC_INSIDETCPSERVER_

#include "TcpServer.hpp"
#include "MessageMgr.hpp"

class InsideTcpServer
{
    public:
        InsideTcpServer();
        ~InsideTcpServer();

        bool InitTcpServer(TcpServer* pTcpServer);
        bool Start();
        bool Stop();
        bool IsRunning();

        static void* DataParser(void* pData);
        static void* MioRun(void *pData);
        static int _c2s_client_mio2_callback(mio2_t m, mio2_action_t a, int fd, void *data, void *arg);

    protected:
        int SimpleHttp(char* pData);
        int RevData(int iFd, int iBytes, mio2_t m);
        int SendData(int iFd, char* pData, int iLen);

    protected:
        c2s_t m_c2s;
        TcpServer* m_pTcpServer;
        pthread_t* m_pProcThread;
        pthread_t* m_pMainThread;
        pthread_mutex_t m_tMutexMio;

    private:
        bool m_bStart;
		pthread_mutex_t m_tMutex;

};

#endif
