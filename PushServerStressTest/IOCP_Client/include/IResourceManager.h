/*
 * File         : IResourceManager.h
 * Data         : 2011-05-19
 * Author       : Heng Qiu
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : drcom client controler
 */
#pragma once
#include <windows.h>
#include "Type.h"

//#include <objbase.h>
//#include <gdiplus.h>
//using namespace Gdiplus;
class IResourceManager
{
	// Construction
protected:
	IResourceManager(void);
	virtual ~IResourceManager(void);

	// Implement
public:
	//* ��ȡʵ������ *//
	static IResourceManager* Instance();
	//* ��ʼ��*//
	virtual BOOL Init(const TCHAR *szSourceFile, const TCHAR *szDestFolder, const TCHAR *szKey = NULL) = 0;
	//* �ͷ�*//
	virtual void Release() = 0;
	//* �����·������ȡ��Դ���е�ͼƬ *//
	virtual HBITMAP GetImage(const TCHAR *szImageFile) = 0; 
	//* �����·������ȡ��Դ���е�ͼ��*//
	virtual HICON GetIcon(const TCHAR *szIconFile, int cx = 0, int cy = 0) = 0;
	//* �����·������ȡ��Դ���ľ���·�� *//
	virtual TSTRING GetTheAbsolutePath(const TCHAR *szResourceFile) = 0; 
	/* GDI��ʽ */
	//virtual Image* GetImage(TCHAR *szImageFile) = 0; 
};
