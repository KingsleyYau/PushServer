// testPushServer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CtestPushServerApp:
// �йش����ʵ�֣������ testPushServer.cpp
//

class CtestPushServerApp : public CWinApp
{
public:
	CtestPushServerApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CtestPushServerApp theApp;