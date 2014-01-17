#include "StdAfx.h"
#include "SSLComm.h"
 
CSSLComm::CSSLComm()
{
    //ctor
    m_sockfd = -1;
    m_pctx = NULL;
    m_pssl = NULL;
    m_pmeth = NULL;
    m_pserver_cert = NULL;
    m_pkey = NULL;

	m_nMsgIndex = 0;
 
	::InitializeCriticalSection(&m_lock);
}
 
CSSLComm::~CSSLComm()
{
    //dtor
    Reset();
	::DeleteCriticalSection(&m_lock);
}
void CSSLComm::Reset()
{
 
    if(m_pssl)
    {
        SSL_shutdown(m_pssl);
        SSL_free(m_pssl);
        m_pssl = NULL;
    }
    if(m_pctx)
    {
        SSL_CTX_free(m_pctx);
        m_pctx = NULL;
    }
    if(m_sockfd > 2)
    {
        closesocket(m_sockfd);
        m_sockfd = -1;
    }
 
}
 
 
bool CSSLComm::connected()
{
    if(m_sockfd < 2) return false;
 
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set fdwrite;
    fd_set fdexcept;
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcept);
    FD_SET(m_sockfd,&fdwrite);
    FD_SET(m_sockfd,&fdexcept);
    int ret = select((int)(m_sockfd)+1,NULL,&fdwrite,&fdexcept,&timeout);
    if(ret == -1)
        return false;
    if(ret > 0)
    {
        if(FD_ISSET(m_sockfd,&fdexcept))
            return false;
        else if(FD_ISSET(m_sockfd,&fdwrite))
        {
            int err = 0;
            int len = sizeof(err);
            int result = getsockopt(m_sockfd,SOL_SOCKET,SO_ERROR,(char*)&err,&len);
            if(result < 0 || err != 0)
                return false;
            return true;
        }
    }
    return false;
}
 
 
bool CSSLComm::ssl_connect(const char *host, int port, const char *certfile, const char *keyfile, const char* capath)
{
    Reset();
 
    int err;
 
    /* Load encryption & hashing algorithms for the SSL program */
    SSL_library_init();
 
    /* Load the error strings for SSL & CRYPTO APIs */
    SSL_load_error_strings();
 
    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
    m_pmeth = SSLv3_method();
 
    /* Create an SSL_CTX structure */
    m_pctx = SSL_CTX_new(m_pmeth);
    if(!m_pctx)
    {
        printf("Could not get SSL Context\n");
        return false;
    }
 
    /* Load the CA from the Path */
    if(SSL_CTX_load_verify_locations(m_pctx, NULL, capath) <= 0)
    {
        /* Handle failed load here */
        printf("Failed to set CA location...\n");
        ERR_print_errors_fp(stderr);
        return false;
    }
 
    /* Load the client certificate into the SSL_CTX structure */
    if (SSL_CTX_use_certificate_file(m_pctx, certfile, SSL_FILETYPE_PEM) <= 0)
    {
        printf("Cannot use Certificate File\n");
        ERR_print_errors_fp(stderr);
        return false;
    }
 
    /* Load the private-key corresponding to the client certificate */
    if (SSL_CTX_use_PrivateKey_file(m_pctx, keyfile, SSL_FILETYPE_PEM) <= 0)
    {
        printf("Cannot use Private Key\n");
        ERR_print_errors_fp(stderr);
        return false;
    }
 
    /* Check if the client certificate and private-key matches */
    if (!SSL_CTX_check_private_key(m_pctx))
    {
        printf("Private key does not match the certificate public key\n");
        return false;
    }
 
    /* Set up a TCP socket */
    m_sockfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_sockfd == -1)
    {
        printf("Could not get Socket\n");
        return false;
    }
 
    memset (&m_server_addr, '\0', sizeof(m_server_addr));
    m_server_addr.sin_family      = AF_INET;
    m_server_addr.sin_port        = htons(port);       /* Server Port number */
    m_phost_info = gethostbyname(host);
    if(m_phost_info)
    {
        /* Take the first IP */
        struct in_addr *address = (struct in_addr*)m_phost_info->h_addr_list[0];
        m_server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
 
    }
    else
    {
        printf("Could not resolve hostname %s\n", host);
        return false;
    }
 
    /* Establish a TCP/IP connection to the SSL client */
    err = connect(m_sockfd, (struct sockaddr*) &m_server_addr, sizeof(m_server_addr));
    if(err == -1)
    {
        printf("Could not connect\n");
        return false;
    }
 
    /* An SSL structure is created */
    m_pssl = SSL_new(m_pctx);
    if(!m_pssl)
    {
        printf("Could not get SSL Socket\n");
        return false;
    }
 
    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(m_pssl, (int)m_sockfd);
 
    /* Perform SSL Handshake on the SSL client */
    err = SSL_connect(m_pssl);
    if(err < 0)
    {
        printf("Could not connect to SSL Server\n");
        return false;
    }
    return true;
 
}

// 验证服务器证书
// 首先要验证服务器的证书有效，其次要验证服务器证书的CommonName(CN)与我们
// 实际要连接的服务器域名一致
int CSSLComm::verify_connection(SSL* ssl, const char* peername)
{
    int result = SSL_get_verify_result(ssl);
    if (result != X509_V_OK) {
        fprintf(stderr, "WARNING! ssl verify failed: %d\n", result);
        return -1;
    }

    X509 *peer;
    char peer_CN[256] = {0};

    peer = SSL_get_peer_certificate(ssl);
    X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN, 255);
    if (strcmp(peer_CN, peername) != 0) {
        fprintf(stderr, "WARNING! Server Name Doesn't match, got: %s, required: %s\n", peer_CN,
            peername);
    }
    return 0;
}
 
bool CSSLComm::PushNotification(const char *pToken, const char *pMsg)
{
    CMyLock lock(&m_lock);
	bool bResult = false;

	if (connected()) {
		// 填入数据
		PUSHDATA pushdata;
		memset(&pushdata, 0, sizeof(pushdata));
		int nDataLen = Gen(pToken, pMsg, pushdata);
		
		// 发送
		//for (int i = 0; i < 10; i++) {
			int ret = SSL_write(m_pssl, (void*)&pushdata, nDataLen);
			printf("ret:%d, nDataLen:%d, Identifier:%d\n", ret, nDataLen, pushdata.nIdentifier);
			if (ret == nDataLen) {
				//// 发送成功
				//unsigned char buffer[1024] = {0};
				//int len = SSL_read(m_pssl, buffer, sizeof(buffer));
				//printf("len:%d\n", len);
				//if (len >= sizeof(PUSHRETURN)) {
				//	PUSHRETURN* pPushReturn = (PUSHRETURN*)buffer;
				//	printf("Command:%d, Status:%d, Identifier:%d\n", pPushReturn->nCommand, pPushReturn->nStatus, pPushReturn->nIdentifier);
				//	switch(pPushReturn->nStatus) 
				//	{
				//	case 0:
				//		printf("Success\n");
				//		break;
				//	case 1:
				//		printf("Processing error\n");
				//		break;
				//	case 2:
				//		printf("Missing device token\n");
				//		break;
				//	case 3:
				//		printf("Missing topic\n");
				//		break;
				//	case 4:
				//		printf("Missing payload\n");
				//		break;
				//	case 5:
				//		printf("Invalid token size\n");
				//		break;
				//	case 6:
				//		printf("Invalid topic size\n");
				//		break;
				//	case 7:
				//		printf("Invalid payload size\n");
				//		break;
				//	case 8:
				//		printf("Invalid token\n");
				//		break;
				//	case 10:
				//		printf("Shutdown\n");
				//		break;
				//	case 255:
				//		printf("None (unknown)\n");
				//		break;
				//	}
				//}
			}
			printf("\n");
			//int ret1 = SSL_read(m_pssl, buffer, 10240);
			//ret = SSL_write(m_pssl, (void*)"123456789", 10);
			//ret = SSL_get_error(m_pssl, ret); //SSL_ERROR_SSL
			bResult = ret > 0;
		//}
	}

	/*int paylen = GenPayloadData(1,pMsg);
    GenPushData(pToken);
    int ret = SSL_write(m_pssl, (void*)&m_data, 35 + paylen);*/

	return bResult;
}

bool CSSLComm::Connect(const char *pCertPath)
{
	bool bConnected = connected();
	if (!bConnected && NULL != pCertPath && pCertPath[0] != '\0') {
		const char *pCertDirPos = strrchr(pCertPath, '\\');
		if (NULL == pCertDirPos) {
			pCertDirPos = strrchr(pCertPath, '/');
		}

		if (NULL != pCertDirPos) {
			char strCertDir[512] = {0};
			memcpy(strCertDir, pCertPath, pCertDirPos - pCertPath);
			bConnected = ssl_connect(APPLE_HOST, APPLE_PORT, pCertPath, pCertPath, strCertDir);
			printf("connect %s:%d result:%d\n", APPLE_HOST, APPLE_PORT, bConnected);
		}
	}

	if (bConnected) {
		verify_connection(m_pssl, APPLE_HOST);
	}
	return bConnected;
}

void CSSLComm::Disconnect()
{
	Reset();
}
 
int CSSLComm::Gen(const char *pToken, const char *pMsg, PUSHDATA& pushdata)
{
	PUSHDATA *pData = (PUSHDATA*)&pushdata;

	// fill
	unsigned int nExpiryTime = (24 * 3600) + (unsigned int)time(NULL);
	pData->nExpiry = htonl(nExpiryTime);	// 过期时间，1天
	pData->nIdentifier = m_nMsgIndex++;							// 消息编号

	// fill payload
	int nMsgLen = (int)strlen(pMsg);
	pData->sPayloadLen = htons(nMsgLen);
	memcpy(pData->szPayload, pMsg, nMsgLen);

	// fill token and command
	pData->cCommand = 1;
	pData->sTokenLen = htons(DEVICE_BINARY_SIZE);
	char tmp[5] = {0};
	for (int i = 0; i < DEVICE_BINARY_SIZE; i++) {
		sprintf_s(tmp, sizeof(tmp), "0x%c%c", pToken[i*2], pToken[(i*2)+1]);
		unsigned char ucTmp = (unsigned char)strtol(tmp, NULL, 16);
		pData->szToken[i] = ucTmp;
	}

	return (sizeof(PUSHDATA) - (MAXPAYLOAD_SIZE - nMsgLen));
}
