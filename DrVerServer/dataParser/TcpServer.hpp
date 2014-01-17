/*
 * File         : TCPServer.hpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server
 */

#ifndef _INC_TCPSERVER_
#define _INC_TCPSERVER_

#include "DataParser.hpp"
#include "RspParser.hpp"
#include "RecvParser.hpp"

class DBSpool;
class TcpServer
{
    public:
        TcpServer();
        ~TcpServer();

        bool Start();
        bool Stop();
        bool ReStart();
        bool IsRunning();
        bool IsRestarting();
//        int InitDBMapping(VerDBMapping* pVerDBMapping);

        //for connect count
        int AddConnect();
        void ReSetConnect();
        int GetConnect();

        static void* DataParse(void* pData);
        static void* RspParse(void* pData);
        static void* RecvParse(void* pData);
        static void* MioRun(void *pData);
        static void* WaitHandle(void* pData);
        /*
        static int _c2s_client_mio_callback(mio_t m, mio_action_t a, int fd, void *data, void *arg);
        */
        static int _c2s_client_mio2_callback(mio2_t m, mio2_action_t a, int fd, void *data, void *arg);

    protected:
        /*
        int RevData(int iFd, mio_t m);
        */
        int RevData(int iFd, mio2_t m);

    protected:
        c2s_t m_c2s;
        DBSpool* m_pDBSpool;
//        VerDBMapping m_VerDBMapping;//just for load db mapping at beginning
        pthread_t* m_pProcThread;
        pthread_t* m_pSndThread;
        pthread_t* m_pRecvThread;
        pthread_t* m_pMainThread;
        pthread_t* m_pWaitThread;
        pthread_mutex_t m_tMutexMio;

    private:
        bool m_bStart, m_bReStart;
//		pthread_mutex_t m_tMutex, m_tMutexConnect;
        pthread_mutex_t m_tMutexConnect;
		int m_iConnect;
};

#endif
