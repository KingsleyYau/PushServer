// PushServerStressTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CPushServerStressTestApp:
// �йش����ʵ�֣������ PushServerStressTest.cpp
//

class CPushServerStressTestApp : public CWinApp
{
public:
	CPushServerStressTestApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPushServerStressTestApp theApp;