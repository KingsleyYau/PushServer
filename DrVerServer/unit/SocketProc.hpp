/*
 * File         : SocketProc.hpp
 * Date         : 2012-11-14
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  :
 */

#ifndef __SOCKETPROC_DEF_H_
#define __SOCKETPROC_DEF_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

inline bool GetRemoteIpAndPort(int iFd, string &ip, int &port)
{
    bool result = false;
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    if (0 == getpeername(iFd, &addr, &addr_len))
    {
        if (addr.sa_family == AF_INET)
        {
             addr_v4 = (sockaddr_in*)&addr;
             ip = inet_ntoa(addr_v4->sin_addr);
             port = ntohs(addr_v4->sin_port);
             result = true;
        }
    }
    return result;
}

#endif // __SOCKETPROC_DEF_H_
