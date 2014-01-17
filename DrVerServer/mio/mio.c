/*
 * jabberd - Jabber Open Source Server
 * Copyright (c) 2002 Jeremie Miller, Thomas Muldowney,
 *                    Ryan Eatmon, Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */

/*
   MIO -- Managed Input/Output
   ---------------------------
*/

#include <stdlib.h>

#include "mio.h"

/** our internal wrapper around a fd */
enum 
{ 
    type_CLOSED = 0x00, 
    type_NORMAL = 0x01, 
    type_LISTEN = 0x02, 
    type_CONNECT = 0x10, 
    type_CONNECT_READ = 0x11,
    type_CONNECT_WRITE = 0x12
};
typedef int mio_type_t;
struct mio_fd_st
{
    int type;
    /* app event handler and data */
    mio_handler_t app;
    void *arg;
};

/** now define our master data type */
struct mio_st
{
    struct mio_fd_st *fds;
    int maxfd;
    int highfd;
    MIO_VARS
};

/* lazy factor */
#define FD(m,f) m->fds[f]
#define ACT(m,f,a,d) (*(FD(m,f).app))(m,a,f,d,FD(m,f).arg)

/* temp debug outputter */
#define ZONE __LINE__

#ifndef MIO_DEBUG
#define MIO_DEBUG 0
#endif

#define mio_debug if(MIO_DEBUG) _mio_debug
void _mio_debug(int line, const char *msgfmt, ...)
{
    char lsBuff[2048];
    memset(lsBuff,0,2048);
    
    va_list ap;
    va_start(ap,msgfmt);
    vsnprintf(lsBuff,2047,msgfmt,ap);
    va_end(ap);
    printf("%d,%s\n",line,lsBuff);
}

MIO_FUNCS

int GetHighfd(mio_t m)
{
	return m->highfd;
}
/** internal close function */
void mio_close(mio_t m, int fd)
{
    if(m->highfd<fd || fd <0 ) return ;
	mio_debug(ZONE,"actually closing fd #%d",fd);

    /* take out of poll sets */
    MIO_REMOVE_FD(m, fd);

    /* let the app know, it must process any waiting write data it has and free it's arg */
    ACT(m, fd, action_CLOSE, NULL);

    /* close the socket, and reset all memory */
    close(fd);
    memset(&FD(m,fd), 0, sizeof(struct mio_fd_st));
}

/** internal close function without call app*/
void mio_close_quick(mio_t m, int fd)
{
    if(m->highfd<fd || fd <0 ) return ;
    mio_debug(ZONE,"actually closing(quick) fd #%d",fd);
    
    /* take out of poll sets */
    MIO_REMOVE_FD(m, fd);

    /* close the socket, and reset all memory */
    close(fd);
    memset(&FD(m,fd), 0, sizeof(struct mio_fd_st));
}
/** internally accept an incoming connection from a listen sock */
void _mio_accept(mio_t m, int fd)
{
    struct sockaddr_storage serv_addr;
    socklen_t addrlen = (socklen_t) sizeof(serv_addr);
    int newfd, dupfd;
    char ip[INET6_ADDRSTRLEN];

    mio_debug(ZONE, "accepting on fd #%d", fd);

    /* pull a socket off the accept queue and check */
    newfd = accept(fd, (struct sockaddr*)&serv_addr, &addrlen);
    if(newfd <= 0) return;
    if(addrlen <= 0) 
    {
        close(newfd);
        return;
    }

    j_inet_ntop(&serv_addr, ip, sizeof(ip));
    mio_debug(ZONE, "new socket accepted fd #%d, %s:%d", newfd, ip, j_inet_getport(&serv_addr));

    /* set up the entry for this new socket */
    if(mio_fd(m, newfd, FD(m,fd).app, FD(m,fd).arg) < 0)
    {
        /* too high, try and get a lower fd */
        dupfd = dup(newfd);
        
         mio_debug(ZONE, "new socket accepted newfd #%d, oldfd #%d", newfd,dupfd);

        close(newfd);

        if(dupfd < 0 || mio_fd(m, dupfd, FD(m,fd).app, FD(m,fd).arg) < 0) 
        {
            mio_debug(ZONE,"failed to add fd");
            if(dupfd >= 0) close(dupfd);
						
            return;
        }

        newfd = dupfd;
    }

    /* tell the app about the new socket, if they reject it clean up */
    if (ACT(m, newfd, action_ACCEPT, ip))
    {
        mio_debug(ZONE, "accept was rejected for %s:%d", ip, newfd);
        MIO_REMOVE_FD(m, newfd);

        /* close the socket, and reset all memory */
        close(newfd);
        memset(&FD(m, newfd), 0, sizeof(mio_fd_st));
        
        return ;
    }
		mio_read(m,newfd);
    return;
}

/** internally change a connecting socket to a normal one */
void _mio_connect(mio_t m, int fd)
{
    mio_type_t type = FD(m,fd).type;

    mio_debug(ZONE, "connect processing for fd #%d", fd);

    /* reset type and clear the "write" event that flags connect() is done */
    FD(m,fd).type = type_NORMAL;
    MIO_UNSET_WRITE(m,fd);

    /* if the app had asked to do anything in the meantime, do those now */
    if(type & type_CONNECT_READ) mio_read(m,fd);
    if(type & type_CONNECT_WRITE) mio_write(m,fd);
}

/** add and set up this fd to this mio */
int mio_fd(mio_t m, int fd, mio_handler_t app, void *arg)
{
    int flags;

    mio_debug(ZONE, "adding fd #%d", fd);

    if(fd >= m->maxfd)
    {
        mio_debug(ZONE,"fd to high,fd=%d,max_fd=%d",fd,m->maxfd);
        return -1;
    }

    /* ok to process this one, welcome to the family */
    FD(m,fd).type = type_NORMAL;
    FD(m,fd).app = app;
    FD(m,fd).arg = arg;
    MIO_INIT_FD(m, fd);

    /* set the socket to non-blocking */
#if defined(HAVE_FCNTL)
    flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
#elif defined(HAVE_IOCTL)
    flags = 1;
    ioctl(fd, FIONBIO, &flags);
#endif

    /* track highest used */
    if(fd > m->highfd) m->highfd = fd;

    return fd;
}

/** reset app stuff for this fd */
void mio_app(mio_t m, int fd, mio_handler_t app, void *arg)
{
    FD(m,fd).app = app;
    FD(m,fd).arg = arg;
}

/** main select loop runner */
void mio_run(mio_t m, int timeout)
{
    int retval, fd;

    mio_debug(ZONE, "highfd=%d,mio running for %d", m->highfd,timeout);
        
    /* wait for a socket event */
    retval = MIO_CHECK(m, timeout);

    /* nothing to do */
    if(retval == 0) {
        return;
    }

    /* an error */
    if(retval < 0)
    {
        mio_debug(ZONE, "MIO_CHECK returned an error (%d,%s)", MIO_ERROR(m),strerror(errno));
        exit(1);
        return;
    }

  //  mio_debug(ZONE,"mio working: %d",retval);

    /* loop through the sockets, check for stuff to do */
    //for(fd = 0; fd <= m->highfd; fd++)
    for(fd = 0; (fd <= m->highfd && retval > 0); fd++)
    {
        /* skip dead slots */
        //if(FD(m,fd).type == type_CLOSED) continue;
        if(FD(m,fd).type == type_CLOSED)
        { 
            if (MIO_CAN_READ(m, fd) || MIO_CAN_WRITE(m, fd)) {
                mio_close_quick(m, fd);
                retval--;
            }
            continue;
        }

        /* new conns on a listen socket */
        if(FD(m,fd).type == type_LISTEN && MIO_CAN_READ(m, fd))
        {
            _mio_accept(m, fd);
            retval--;
            continue;
        }

        /* check for connecting sockets */
        if(FD(m,fd).type & type_CONNECT && (MIO_CAN_READ(m, fd) || MIO_CAN_WRITE(m, fd)))
        {
            _mio_connect(m, fd);
            retval--;
            continue;
        }

        /* read from ready sockets */
        if(FD(m,fd).type == type_NORMAL && MIO_CAN_READ(m, fd))
        {
            /* if they don't want to read any more right now */
            if(ACT(m, fd, action_READ, NULL) == 0) {
                //MIO_UNSET_READ(m, fd);
                mio_close_quick(m, fd);
            }
            retval--;
        }

        /* write to ready sockets */
        if(FD(m,fd).type == type_NORMAL && MIO_CAN_WRITE(m, fd))
        {
            /* don't wait for writeability if nothing to write anymore */
            if(ACT(m, fd, action_WRITE, NULL) == 0)
                MIO_UNSET_WRITE(m, fd);
            retval--;
        }
    } 
}

/** eve */
mio_t mio_new(int maxfd)
{
    mio_t m;

    /* allocate and zero out main memory */
    m =(mio_t)malloc(sizeof(mio_st));
    if(m == NULL) return NULL;
    
    m->fds =(mio_fd_st *)malloc(sizeof(mio_fd_st) * maxfd);
    if(m->fds == NULL)
    {
        mio_debug(ZONE,"internal error creating new mio,size=%d,max=%ld",sizeof(mio_fd_st),maxfd);
        free(m);
        return NULL;
    }
    mio_debug(ZONE," creating new mio buff =%d",sizeof(mio_fd_st) * maxfd);
    memset(m->fds, 0, sizeof(mio_fd_st) * maxfd);

    /* set up our internal vars */
    m->maxfd = maxfd;
    m->highfd = 0;

    MIO_INIT_VARS(m);

    return m;
}

/** adam */
void mio_free(mio_t m)
{
    MIO_FREE_VARS(m);

    free(m->fds);
    
    free(m);
}

/** start processing read events */
void mio_read(mio_t m, int fd)
{
    if(m == NULL || fd < 0) return;

    /* if connecting, do this later */
    if(FD(m,fd).type & type_CONNECT)
    {
        FD(m,fd).type |= type_CONNECT_READ;
        return;
    }

    MIO_SET_READ(m, fd);
}

/** try writing to the socket via the app */
void mio_write(mio_t m, int fd)
{
    if(m == NULL || fd < 0) return;

    /* if connecting, do this later */
    if(FD(m,fd).type & type_CONNECT)
    {
        FD(m,fd).type |= type_CONNECT_WRITE;
        return;
    }

    if(FD(m,fd).type != type_NORMAL)
        return;

    if(ACT(m, fd, action_WRITE, NULL) == 0) return;

    /* not all written, do more l8r */
    MIO_SET_WRITE(m, fd);
}

/** set up a listener in this mio w/ this default app/arg */
int mio_listen(mio_t m, int port, char *sourceip, mio_handler_t app, void *arg)
{
    int fd, flag = 1;
    struct sockaddr_storage sa;

    if(m == NULL) return -1;

    mio_debug(ZONE, "mio to listen on %d [%s]", port, sourceip);

    memset(&sa, 0, sizeof(sa));

    /* if we specified an ip to bind to */
    if(sourceip != NULL && !j_inet_pton(sourceip, &sa))
        return -1;

    if(sa.ss_family == 0)
        sa.ss_family = AF_INET;
    
    /* attempt to create a socket */
    if((fd = socket(sa.ss_family,SOCK_STREAM,0)) < 0) return -1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0) return -1;

    /* set up and bind address info */
    j_inet_setport(&sa, port);
    if(bind(fd,(struct sockaddr*)&sa,j_inet_addrlen(&sa)) < 0)
    {
        close(fd);
        return -1;
    }

    /* start listening with a max accept queue of 10 */
    if(listen(fd, 10) < 0)
    {
        close(fd);
        return -1;
    }

    /* now set us up the bomb */
    if(mio_fd(m, fd, app, arg) < 0)
    {
        close(fd);
        return -1;
    }
    FD(m,fd).type = type_LISTEN;
    /* by default we read for new sockets */
    mio_read(m,fd);

    return fd;
}

/** create an fd and connect to the given ip/port */
int mio_connect(mio_t m, int port, char *hostip, mio_handler_t app, void *arg)
{
    int fd, flag, flags;
    struct sockaddr_storage sa;

    memset(&sa, 0, sizeof(sa));

    if(m == NULL || port <= 0 || hostip == NULL) return -1;

    mio_debug(ZONE, "mio connecting to %s, port=%d",hostip,port);

    /* convert the hostip */
    if(j_inet_pton(hostip, &sa)<=0)
        return -1;

    /* attempt to create a socket */
    if((fd = socket(sa.ss_family,SOCK_STREAM,0)) < 0) return -1;

    /* set the socket to non-blocking before connecting */
#if defined(HAVE_FCNTL)
    flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
#elif defined(HAVE_IOCTL)
    flags = 1;
    ioctl(fd, FIONBIO, &flags);
#endif

    /* set up address info */
    j_inet_setport(&sa, port);

    /* try to connect */
    flag = connect(fd,(struct sockaddr*)&sa,j_inet_addrlen(&sa));

    mio_debug(ZONE, "connect returned %d and %s", flag, strerror(errno));

    /* already connected?  great! */
    if(flag == 0 && mio_fd(m,fd,app,arg) == fd) return fd;

    /* gotta wait till later */
    if(flag == -1 && errno == EINPROGRESS && mio_fd(m,fd,app,arg) == fd)
    {
        mio_debug(ZONE, "connect processing non-blocking mode");

        FD(m,fd).type = type_CONNECT;
        MIO_SET_WRITE(m,fd);
        return fd;
    }

    /* bummer dude */
    close(fd);
    return -1;
}

