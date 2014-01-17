// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// 从 Windows 头中排除极少使用的资料
#endif

// 如果您必须使用下列所指定的平台之前的平台，则修改下面的定义。
// 有关不同平台的相应值的最新信息，请参考 MSDN。
#ifndef WINVER				// 允许使用特定于 Windows XP 或更高版本的功能。
#define WINVER 0x0501		// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0501	// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif						

#ifndef _WIN32_WINDOWS		// 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0410 // 将它更改为适合 Windows Me 或更高版本的相应值。
#endif

#ifndef _WIN32_IE			// 允许使用特定于 IE 6.0 或更高版本的功能。
#define _WIN32_IE 0x0600	// 将此值更改为相应的值，以适用于 IE 的其他版本。值。
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展


#include <afxdisp.h>        // MFC 自动化类



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <afxsock.h>		// MFC 套接字扩展
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

//======出错返回代码具体含义=======//
#define FAIL_CONNECT_PUSHSRV 0x00000001				// 连接PushServer失败
#define FAIL_GET_CHALLENGE		  0x00000002				// getchallenge失败
#define FAIL_REGISTER_TOKENID 0x00000003				// 注册tokenid失败
#define FAIL_GET_PUSHMESSAGE 0x00000004				// getpushmessage失败	
#define FAIL_SET_PUSHMESSAGE  0x00000005				// setpushmessage失败
#define SOCKET_CLOSE				0x00000020

// 定义数据缓冲区大小
#define DATA_BUF_SIZE 1024



// 定义客户端的执行进度
enum Perform_Progress{						// 执行到哪个步骤
	NON_EXEC = 0,									// 未执行
//	ESTABLISH_CONNECT,						// 建立与服务器的连接
//	CONNECT_SUCCESS,							// 连接成功
	SEND_GETCHALLENGE,						// 发出GetChallenge
	RECV_GETCHALLENGE,						//  GetChallenge返回成功
	SEND_REGISTER_TOKENID,					// 发出Register TokenID
	RECV_REGESTER_TOKENID,					// Register TokenID返回成功
	SEND_GETPUSHMESSAGE,					// 发送PushMessage
	RECV_GETPUSHMESSAGE,					// PushMessage返回成功
	RECV_PUSHMESSAGE,						// 开始接收PushMessage
	TERMINATE_CONNECT,
};

// I/O操作相关数据结构
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