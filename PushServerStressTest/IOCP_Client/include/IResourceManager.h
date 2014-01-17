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
	//* 获取实例对象 *//
	static IResourceManager* Instance();
	//* 初始化*//
	virtual BOOL Init(const TCHAR *szSourceFile, const TCHAR *szDestFolder, const TCHAR *szKey = NULL) = 0;
	//* 释放*//
	virtual void Release() = 0;
	//* 以相对路径，获取资源包中的图片 *//
	virtual HBITMAP GetImage(const TCHAR *szImageFile) = 0; 
	//* 以相对路径，获取资源包中的图标*//
	virtual HICON GetIcon(const TCHAR *szIconFile, int cx = 0, int cy = 0) = 0;
	//* 以相对路径，获取资源包的绝对路径 *//
	virtual TSTRING GetTheAbsolutePath(const TCHAR *szResourceFile) = 0; 
	/* GDI方式 */
	//virtual Image* GetImage(TCHAR *szImageFile) = 0; 
};
