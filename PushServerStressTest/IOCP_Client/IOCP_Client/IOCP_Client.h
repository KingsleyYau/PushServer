// IOCP_Client.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CIOCP_ClientApp:
// �йش����ʵ�֣������ IOCP_Client.cpp
//

class CIOCP_ClientApp : public CWinApp
{
public:
	CIOCP_ClientApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CIOCP_ClientApp theApp;