/*
 * Date         : 2012-01-05
 * Author       : Keqin Su
 * File         : mio2_epoll.c
 * Description  : epoll implement
 */

#include <stdarg.h>
#include "inaddr.h"
#include "mio2_epoll.hpp"
#include "PushManager.hpp"
/// fgx 2012-12-12 延迟closesocket
#include <deque>
#include <pthread.h>
#include "TimeProc.hpp"

#define ACT(m, f, a, d) (*(m->app))(m, a, f, d, m->arg)

#ifndef MIO2_DEBUG
#define MIO2_DEBUG 0
#endif

/* temp debug outputter */
#define ZONE __LINE__

/// -----=== fgx 2012-12-12 ===----
/// 延迟closesocket，延长socket值重用时间
typedef struct _tagCloseSocketItem
{
    int     iFd;
    time_t  tApplyCloseTime;
}TCloseSocketItem;
typedef deque<TCloseSocketItem> CloseSocketQueue;

static bool             g_isCloseSocketRun;
static pthread_t        g_closeSocketThread;    // 关闭socket线程
static CloseSocketQueue g_closeSocketQueue; // 关闭socket队列
static pthread_mutex_t  g_mutexCloseSocket; // 关闭socket队列锁

#define CLOSESOCKET_TIME    1000     // 1秒后关闭
/// --------------------------------

#define mio2_debug if(MIO2_DEBUG) _mio2_debug
void _mio2_debug(int line, const char *msgfmt, ...)
{
    char lsBuff[2048];
    memset(lsBuff, 0, 2048);

    va_list ap;
    va_start(ap, msgfmt);
    vsnprintf(lsBuff, 2047, msgfmt, ap);
    va_end(ap);
    printf("%d,%s\n",line, lsBuff);
}

mio2_t mio2_new(int maxfd)
{
    mio2_t m = NULL;

    /// fgx 2012-12-12 新建延迟关闭socket线程
    pthread_mutex_init(&g_mutexCloseSocket, NULL);
    g_isCloseSocketRun = true;
    pthread_create(&g_closeSocketThread, NULL, &CloseSocketThreadProc, NULL);

    /* allocate and zero out main memory */
    m = (mio2_t)malloc(sizeof(mio2_st));
    if(m == NULL) return NULL;
    memset(m, 0, sizeof(mio2_st));

    m->maxfd = maxfd;
    m->epollfd = epoll_create(maxfd);
    if (m->epollfd < 0) {
        free(m);
        return NULL;
    }

    m->pevn =(epoll_event*)malloc(sizeof(epoll_event) * (SOMAXCONN + 1));
    if(m->pevn == NULL) {
        close(m->epollfd);
        free(m);
        return NULL;
    }
    memset(m->pevn, 0, sizeof(epoll_event) * (SOMAXCONN + 1));
    pthread_mutex_init(&(m->mtx), NULL);
    return m;
}

void mio2_free(mio2_t m)
{
    if (m && (m->epollfd > 0) && (m->pevn != NULL)) {
        close(m->epollfd);
        free(m->pevn);
        pthread_mutex_destroy(&(m->mtx));
        free(m);
    }

    g_isCloseSocketRun = false;
    pthread_join(g_closeSocketThread, NULL);
    pthread_mutex_destroy(&g_mutexCloseSocket);
}

int mio2_listen(mio2_t m, int port, char *sourceip, mio2_handler_t app, void *arg)
{
    int fd, flag = 1;
    struct sockaddr_storage sa;

    if (m == NULL) return -1;

    memset(&sa, 0, sizeof(sa));

    /* if we specified an ip to bind to */
    if (sourceip != NULL && !j_inet_pton(sourceip, &sa))
        return -1;

    if (sa.ss_family == 0)
        sa.ss_family = AF_INET;

    /* attempt to create a socket */
    if ((fd = socket(sa.ss_family, SOCK_STREAM, 0)) < 0) return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0) return -1;

    flag = fcntl(fd, F_GETFL);
    flag = flag | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) < 0 ) return -1;

    /* set up and bind address info */
    j_inet_setport(&sa, port);
    if (bind(fd, (struct sockaddr*)&sa, j_inet_addrlen(&sa)) < 0) {
        close(fd);
        return -1;
    }

    /* start listening with a max accept queue of SOMAXCONN */
    if(listen(fd, SOMAXCONN) < 0) {
        close(fd);
        return -1;
    }

    m->pevn[SOMAXCONN].events = EPOLLIN|EPOLLERR;
    m->pevn[SOMAXCONN].data.fd = fd;
    m->app = app;
    m->arg = arg;
    if (epoll_ctl(m->epollfd, EPOLL_CTL_ADD, m->pevn[SOMAXCONN].data.fd, &(m->pevn[SOMAXCONN])) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

void mio2_close(mio2_t m, int fd)
{
    if (m == NULL || fd < 0 || m->epollfd == 0) return;

//    gvLog(LOG_MSG, "(mio2_close) OnCloseSocket() begin");
//    OnCloseSocket(fd);
    pthread_mutex_lock(&(m->mtx));
    /* remove from epfd */
    epoll_ctl(m->epollfd, EPOLL_CTL_DEL, fd, NULL);
    /* close the socket */
    shutdown(fd, SHUT_RDWR);

    /// ----==== fgx 2012-12-12 ====----
    /// 改为延迟关闭socket
    //close(fd);
    TCloseSocketItem closeSocketItem;
    closeSocketItem.iFd = fd;
    closeSocketItem.tApplyCloseTime = GetTickCount();
    pthread_mutex_lock(&g_mutexCloseSocket);
    g_closeSocketQueue.push_back(closeSocketItem);
    pthread_mutex_unlock(&g_mutexCloseSocket);
    /// --------------------------------

    pthread_mutex_unlock(&(m->mtx));
}

void mio2_close_quick(mio2_t m, int fd)
{
    if (m == NULL || fd < 0 || m->epollfd == 0) return;

    pthread_mutex_lock(&(m->mtx));
    /* remove from epfd */
    epoll_ctl(m->epollfd, EPOLL_CTL_DEL, fd, NULL);
    /* close the socket */
    shutdown(fd, SHUT_RDWR);

    /// ----==== fgx 2012-12-12 ====----
    /// 改为延迟关闭socket
    //close(fd);
    TCloseSocketItem closeSocketItem;
    closeSocketItem.iFd = fd;
    closeSocketItem.tApplyCloseTime = GetTickCount();
    pthread_mutex_lock(&g_mutexCloseSocket);
    g_closeSocketQueue.push_back(closeSocketItem);
    pthread_mutex_unlock(&g_mutexCloseSocket);
    /// --------------------------------

    pthread_mutex_unlock(&(m->mtx));
}

int _mio2_accept(mio2_t m, int fd)
{
    struct sockaddr_storage serv_addr;
    socklen_t addrlen = (socklen_t) sizeof(serv_addr);
    int newfd = -1;
    char ip[INET6_ADDRSTRLEN] = {'\0'};
    /*deal with the tcp keepalive
      iKeepIdle = 600 (active keepalive after socket has idled for 10 minutes)
      KeepInt = 60 (send keepalive every 1 minute after keepalive was actived)
      iKeepCount = 3 (send keepalive 3 times before disconnect from peer)
     */
    int iKeepAlive = 1, iKeepIdle = 180, KeepInt = 60, iKeepCount = 3;

    /* pull a socket off the accept queue and check */
    newfd = accept(fd, (struct sockaddr*)&serv_addr, &addrlen);
    if (newfd <= 0) {
        return 0;
    }
    if (addrlen <= 0) {
        close(newfd);
        return 0;
    }

    if (fcntl(newfd, F_SETFL, O_NONBLOCK) < 0 ) {
        close(newfd);
        return 0;
    }

    if (setsockopt(newfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0) {
        close(newfd);
        return 0;
    }

    if (setsockopt(newfd, SOL_TCP, TCP_KEEPIDLE, (void*)&iKeepIdle, sizeof(iKeepIdle)) < 0) {
        close(newfd);
        return 0;
    }

    if (setsockopt(newfd, SOL_TCP, TCP_KEEPINTVL, (void *)&KeepInt, sizeof(KeepInt)) < 0) {
        close(newfd);
        return 0;
    }

    if (setsockopt(newfd, SOL_TCP, TCP_KEEPCNT, (void *)&iKeepCount, sizeof(iKeepCount)) < 0) {
        close(newfd);
        return 0;
    }

    j_inet_ntop(&serv_addr, ip, sizeof(ip));
    if (ACT(m, newfd, mio2_action_ACCEPT, ip) == 0) {
        /* close the socket, and reset all memory */
        close(newfd);
        return 0;
    }
    return newfd;
}

void mio2_run(mio2_t m, int timeout)
{
    int retval = -1, fd = -1, newfd = -1, i = 0;

    /* wait for a socket event */
    retval = epoll_wait(m->epollfd, m->pevn, SOMAXCONN, timeout * 1000);

    if (retval == 0) {
        return;
    }

    if (retval < 0) {
        exit(1);
        return;
    }

    for (i = 0; i < retval; i++) {
        /* new conns on a listen socket */
        if (m->pevn[i].data.fd == m->pevn[SOMAXCONN].data.fd) {
            if (m->pevn[i].events & EPOLLIN) {
                newfd = _mio2_accept(m, m->pevn[SOMAXCONN].data.fd);
                if (newfd > 0) {
                    m->pevn[i].events = EPOLLIN|EPOLLPRI|EPOLLERR|EPOLLHUP;
                    m->pevn[i].data.fd = newfd;
                    epoll_ctl(m->epollfd, EPOLL_CTL_ADD, m->pevn[i].data.fd, &(m->pevn[i]));
                }
            } else if (m->pevn[i].events & EPOLLERR) {
                /* while listen socket error appear, modify again. I hope it will work */
                epoll_ctl(m->epollfd, EPOLL_CTL_MOD, m->pevn[SOMAXCONN].data.fd, &(m->pevn[SOMAXCONN]));
                mio2_debug(ZONE,"listen socket error fd=%d", m->pevn[SOMAXCONN].data.fd);
            }
        } else {
            if (m->pevn[i].events & EPOLLIN|EPOLLPRI) {
                //available socket for read
                if (ACT(m, m->pevn[i].data.fd, mio2_action_READ, NULL) == 0) {
                    mio2_close(m, m->pevn[i].data.fd);
                    continue;
                }
            } else if (m->pevn[i].events & EPOLLERR|EPOLLHUP) {
                //unavailable socket
                mio2_close(m, m->pevn[i].data.fd);
                continue;
            } else if (m->pevn[i].events & EPOLLOUT) {
                //available socket for send
                if (ACT(m, m->pevn[i].data.fd, mio2_action_WRITE, NULL) == 0) {
                    mio2_close(m, m->pevn[i].data.fd);
                    continue;
                }
            }
        }
    }
}

/// fgx 2012-12-12 延迟关闭socket线程
void* CloseSocketThreadProc(void* pObj)
{
    TCloseSocketItem item;
    while (g_isCloseSocketRun)
    {
        pthread_mutex_lock(&g_mutexCloseSocket);
        while (g_closeSocketQueue.size() > 0)
        {
            item = g_closeSocketQueue.front();
            if (DiffGetTickCount(item.tApplyCloseTime, GetTickCount()) >= CLOSESOCKET_TIME)
            {
                OnCloseSocket(item.iFd);
                close(item.iFd);
                g_closeSocketQueue.pop_front();
            }
            else
            {
                // 没到时间
                break;
            }
        }
        pthread_mutex_unlock(&g_mutexCloseSocket);
        sleep(1);
    }

    // 清空关闭队列
    while (g_closeSocketQueue.size() > 0)
    {
        item = g_closeSocketQueue.front();
        close(item.iFd);
        g_closeSocketQueue.pop_front();
    }
    return NULL;
}
