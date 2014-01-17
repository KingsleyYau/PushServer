/*
 * Date         : 2012-01-05
 * Author       : Keqin Su
 * File         : mio2_epoll.hpp
 * Description  : epoll implement
 */

#ifndef _INC_MIO2_EPOLL_
#define _INC_MIO2_EPOLL_

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>

//#ifdef __cplusplus
//extern "C" {
//#endif

typedef struct mio2_st *mio2_t;
typedef enum { mio2_action_ACCEPT, mio2_action_READ, mio2_action_WRITE, mio2_action_CLOSE } mio2_action_t;
typedef int (*mio2_handler_t) (mio2_t m, mio2_action_t a, int fd, void* data, void *arg);

struct mio2_st
{
    epoll_event* pevn;
    int epollfd;
    int maxfd;

    mio2_handler_t app;
    void *arg;
    pthread_mutex_t mtx;

};

mio2_t mio2_new(int maxfd);
void mio2_free(mio2_t m);

int mio2_listen(mio2_t m, int port, char *sourceip, mio2_handler_t app, void *arg);
void mio2_close(mio2_t m, int fd);
void mio2_close_quick(mio2_t m, int fd);
void mio2_run(mio2_t m, int timeout);

/// fgx 2012-12-12 延迟关闭socket线程
void* CloseSocketThreadProc(void* pObj);

//#ifdef __cplusplus
//}
//#endif

#endif
