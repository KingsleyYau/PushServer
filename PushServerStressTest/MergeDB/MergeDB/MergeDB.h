// MergeDB.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������

// CMergeDBApp:
// �йش����ʵ�֣������ MergeDB.cpp
//

class CMergeDBApp : public CWinApp
{
public:
	CMergeDBApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMergeDBApp theApp;