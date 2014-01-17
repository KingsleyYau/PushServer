// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// �� Windows ͷ���ų�����ʹ�õ�����
#endif

// ���������ʹ��������ָ����ƽ̨֮ǰ��ƽ̨�����޸�����Ķ��塣
// �йز�ͬƽ̨����Ӧֵ��������Ϣ����ο� MSDN��
#ifndef WINVER				// ����ʹ���ض��� Windows XP ����߰汾�Ĺ��ܡ�
#define WINVER 0x0501		// ����ֵ����Ϊ��Ӧ��ֵ���������� Windows �������汾��
#endif

#ifndef _WIN32_WINNT		// ����ʹ���ض��� Windows XP ����߰汾�Ĺ��ܡ�
#define _WIN32_WINNT 0x0501	// ����ֵ����Ϊ��Ӧ��ֵ���������� Windows �������汾��
#endif						

#ifndef _WIN32_WINDOWS		// ����ʹ���ض��� Windows 98 ����߰汾�Ĺ��ܡ�
#define _WIN32_WINDOWS 0x0410 // ��������Ϊ�ʺ� Windows Me ����߰汾����Ӧֵ��
#endif

#ifndef _WIN32_IE			// ����ʹ���ض��� IE 6.0 ����߰汾�Ĺ��ܡ�
#define _WIN32_IE 0x0600	// ����ֵ����Ϊ��Ӧ��ֵ���������� IE �������汾��ֵ��
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// ĳЩ CString ���캯��������ʽ��

// �ر� MFC ��ĳЩ�����������ɷ��ĺ��Եľ�����Ϣ������
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ


#include <afxdisp.h>        // MFC �Զ�����



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <afxsock.h>		// MFC �׽�����չ
#undef T2W
#undef W2T

//#ifdef _DEBUG
//#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
//#else
//#define DEBUG_CLIENTBLOCK
//#endif  // _DEBUG
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//#ifdef _DEBUG
//#define new DEBUG_CLIENTBLOCK
//#endif  // _DEBUG

//////////////////////////////////////////////////////////////////////////

//======�����ش�����庬��=======//
#define FAIL_CONNECT_PUSHSRV 0x00000001				// ����PushServerʧ��
#define FAIL_GET_CHALLENGE		  0x00000002				// getchallengeʧ��
#define FAIL_REGISTER_TOKENID 0x00000003				// ע��tokenidʧ��
#define FAIL_GET_PUSHMESSAGE 0x00000004				// getpushmessageʧ��	
#define FAIL_SET_PUSHMESSAGE  0x00000005				// setpushmessageʧ��
#define SOCKET_CLOSE				0x00000020

// �������ݻ�������С
#define DATA_BUF_SIZE 1024



// ����ͻ��˵�ִ�н���
enum Perform_Progress{						// ִ�е��ĸ�����
	NON_EXEC = 0,									// δִ��
//	ESTABLISH_CONNECT,						// �����������������
//	CONNECT_SUCCESS,							// ���ӳɹ�
	SEND_GETCHALLENGE,						// ����GetChallenge
	RECV_GETCHALLENGE,						//  GetChallenge���سɹ�
	SEND_REGISTER_TOKENID,					// ����Register TokenID
	RECV_REGESTER_TOKENID,					// Register TokenID���سɹ�
	SEND_GETPUSHMESSAGE,					// ����PushMessage
	RECV_GETPUSHMESSAGE,					// PushMessage���سɹ�
	RECV_PUSHMESSAGE,						// ��ʼ����PushMessage
	TERMINATE_CONNECT,
};

// I/O����������ݽṹ
typedef struct _io_operation_data
{
	OVERLAPPED		overlapped;
	WSABUF			wsaDataBuf;
	char					szBuf[DATA_BUF_SIZE];
	BYTE					optype;
	BYTE					len;

	struct _io_operation_data()
		:optype(0)
		,len(0)
	{
		ZeroMemory(szBuf, sizeof(szBuf));
	}
}IO_OPERATION_DATA, *PIO_OPERATION_DATA;



//////////////////////////////////////////////////////////////////////////


//#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
//#endif