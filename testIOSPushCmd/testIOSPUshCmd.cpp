// testIOSPUshCmd.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Winsock2.h>
#include "PathProc.h"

// 使用开发证书
//#define APNS_DEV

#ifdef APNS_DEV
	#define DEV_TYPE_SIGN	"develop"
#else
	#define DEV_TYPE_SIGN	"production"
#endif

#include "SSLComm.h"

int _tmain(int argc, _TCHAR* argv[])
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData );

	//const char *pCertPath = "pro_push_ICT.pem";		// production
	//const char *pCertPath = "dev_push_ICT.pem";		// develop
	const char *pCertPath = "pro_ck_9587.pem";
	//const char *pCertPath = "pro_push_donghui.pem";
	std::string strCertPath = GetAbsolutePath(".\\pem\\");
	strCertPath += pCertPath;

	const char *pTokenID = "c0344f1abb7ef8a48244cef5229d093c462821db374ea5b6e76398bbe77c6fd4";	// fgx 4s production
	//const char *pTokenID = "961738d225fee4d815676d223b320798367887e730aeb8f282413472a6b0d806";	// fgx 4s develop

	const char *pMsgData = "{\"aps\":{\"alert\":\"Push Test %s %d.\",\"badge\":%d}}";
	char cMessage[1024] = {0};
	int iMsgIndex = 1;

	CSSLComm sslComm;
	sslComm.Connect(strCertPath.c_str());
	sprintf_s(cMessage, sizeof(cMessage), pMsgData, DEV_TYPE_SIGN, iMsgIndex++, iMsgIndex);
	sslComm.PushNotification(pTokenID, cMessage);
	sprintf_s(cMessage, sizeof(cMessage), pMsgData, DEV_TYPE_SIGN, iMsgIndex++, iMsgIndex);
	sslComm.PushNotification(pTokenID, cMessage);
	sprintf_s(cMessage, sizeof(cMessage), pMsgData, DEV_TYPE_SIGN, iMsgIndex++, iMsgIndex);
	sslComm.PushNotification(pTokenID, cMessage);
	sprintf_s(cMessage, sizeof(cMessage), pMsgData, DEV_TYPE_SIGN, iMsgIndex++, iMsgIndex);
	sslComm.PushNotification(pTokenID, cMessage);
	sslComm.Disconnect();
	getchar();
	return 0;
}

