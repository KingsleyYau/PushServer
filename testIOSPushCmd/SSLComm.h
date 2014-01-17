#pragma once

#ifndef SSLCOMM_H
#define SSLCOMM_H
 
#pragma pack(1)

//#ifdef linux
//#include <assert.h>
#include <Winsock2.h>
#include "openssl/pem.h"
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"
 
//#include "errno.h"
//#include "sys/socket.h"
//#include "netinet/in.h"
//#include "unistd.h"
//#include <arpa/inet.h>
//#include <netdb.h>
// 
//#include "Utility.h"
 
#if defined(APNS_DEV)
/* Development Connection Infos */
#define APPLE_HOST          "gateway.sandbox.push.apple.com"
#define APPLE_PORT          2195
 
#define APPLE_FEEDBACK_HOST "feedback.sandbox.push.apple.com"
#define APPLE_FEEDBACK_PORT 2196
 
#else
#define APPLE_HOST          "gateway.push.apple.com"
#define APPLE_PORT          2195
#define APPLE_FEEDBACK_HOST "feedback.push.apple.com"
#define APPLE_FEEDBACK_PORT 2196
#endif

#define CA_CERT_PATH    ".\\pem"
#define RSA_CLIENT_CERT     ".\\pem\\apns-pro-cer_ICT.pem"
#define RSA_CLIENT_KEY      ".\\pem\\apns-pro-cer_ICT.pem"

class CMyLock
{
public:
	CMyLock(LPCRITICAL_SECTION pLock) {
		m_pLock = pLock;
		EnterCriticalSection(m_pLock);
	}
	
	~CMyLock() {
		LeaveCriticalSection(m_pLock);
		m_pLock = NULL;
	}
private:
	LPCRITICAL_SECTION	m_pLock;
};

#define DEVICE_BINARY_SIZE	32		// token����
#define MAXPAYLOAD_SIZE		256		// jsonЭ�鳤��
class CSSLComm
{
public:
	struct PUSHDATA		// ���Ͱ�
    {
		unsigned char cCommand;				// ����λ��Ĭ��=1
		unsigned int nIdentifier;			// ��Ϣ���
		unsigned int nExpiry;				// ��Ϣ����ʱ��
		unsigned short sTokenLen;			// Token����
        unsigned char szToken[DEVICE_BINARY_SIZE];	// Token
		unsigned short sPayloadLen;			// jsonЭ�鳤��
        char szPayload[256];				// jsonЭ��
    };

	struct PUSHRETURN	// Apple Push���������ذ�
	{
		unsigned char nCommand;				// ����λ��Ĭ��=8
		unsigned char nStatus;				// ״̬λ����ʾ�Ƿ��ͳɹ���http://developer.apple.com/library/mac/#documentation/NetworkingInternet/Conceptual/RemoteNotificationsPG/Chapters/CommunicatingWIthAPS.html#//apple_ref/doc/uid/TP40008194-CH101-SW1
		unsigned int nIdentifier;			// ��Ϣ��ţ��� PUSHDATA �� nIdentifier ��Ӧ
	};

public:
    CSSLComm();
    ~CSSLComm();
 
public:
	bool Connect(const char *pCertPath);
	void Disconnect();
    bool PushNotification(const char *pToken, const char *pMsg);
 
private:
	bool connected();
    bool ssl_connect(const char *host, int port, const char *certfile, const char *keyfile, const char* capath);
	int CSSLComm::verify_connection(SSL* ssl, const char* peername);
    void Reset();
	int Gen(const char *pToken, const char *pMsg, PUSHDATA& pushdata);
 
private:
 
    SSL_CTX         *m_pctx;
    SSL             *m_pssl;
    const SSL_METHOD      *m_pmeth;
    X509            *m_pserver_cert;
    EVP_PKEY        *m_pkey;
 
    /* Socket Communications */
    struct sockaddr_in   m_server_addr;
    struct hostent      *m_phost_info;
 
    SOCKET                  m_sockfd;
	int m_nMsgIndex;	// ��Ϣ���
 
    CRITICAL_SECTION m_lock;
};
 
//#endif
 
#endif // SSLCOMM_H
