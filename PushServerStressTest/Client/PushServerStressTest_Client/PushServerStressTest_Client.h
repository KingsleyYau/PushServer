// PushServerStressTest_Client.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "ClientDB.h"


// CPushServerStressTest_ClientApp:
// �йش����ʵ�֣������ PushServerStressTest_Client.cpp
//

class CPushServerStressTest_ClientApp : public CWinApp
{
public:
	CPushServerStressTest_ClientApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPushServerStressTest_ClientApp theApp;