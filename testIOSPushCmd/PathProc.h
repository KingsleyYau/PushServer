/*
 * File         : PathProc.h
 * Date         : 2011-06-07
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : 路径处理
 */

#pragma once
#include <string>
#include <windows.h>

#define TString std::string
#define TSTRING TString

// 获取当前目录路径
inline TString GetCurrPath()
{
	TCHAR tcPath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, tcPath, MAX_PATH);
	TCHAR* pCut = _tcsrchr(tcPath, '\\');
	if (pCut)
	{
		pCut[1] = '\0';
	}

	return TString(tcPath);
}

static TString GetAbsolutePath(const TString& tsRelativePath)
{
	TString tsCurrPath = GetCurrPath();
	TString tsRelative(tsRelativePath);
	TString tsTemp;
	TString tsResult;

	tsResult = tsCurrPath;
	if(0 == tsRelative.length())
		return tsCurrPath;
	switch(tsRelative.at(0))
	{
	case '.':
		{
			// .\开头
			if(tsRelative.at(1) == '\\')
			{
				tsTemp = tsRelative.substr(2,tsCurrPath.length() - 2);
				tsResult += tsTemp;
				break;
			}
			// ./开头
			else if(tsRelative.at(1) == '/')
			{
				tsTemp = tsRelative.substr(2,tsCurrPath.length() - 2);
				tsResult += tsTemp;
				break;
			}
			// ..\开头
			else	
				tsResult += tsRelative;/* 跳转到其他情况 */
		}break;
	case '\\':  // \开头
		{
			tsTemp = tsRelative.substr(1,tsCurrPath.length() - 1);
			tsResult += tsTemp;
			break;
		}break;
	default:   // 其他情况 a* 
		{
			tsResult += tsRelative;
		}break;
	}

	return tsResult;
}

// 输入相对路径获取绝对路径
inline TString GetAbsolutePath(const TCHAR* cRelativePath)
{
	return GetAbsolutePath(TString(cRelativePath));
}

// 获取文件名(去除目录，如：输入c:\abc.txt，输出abc.txt)
inline TString GetFileNameWithPath(const TCHAR *szFilePath)
{
	TString tstrName = szFilePath;
	// 去目录
	size_t nDirPos = tstrName.find_last_of(_T('\\'));
	if (TString::npos != nDirPos){
		tstrName.erase(0, nDirPos+1);
	}
	return tstrName;
}

// 获取没有后缀的文件名(去除目录，如：输入c:\abc.txt，输出abc)
inline TString GetFileNameWithoutExt(const TCHAR *szFilePath)
{
	TString tstrName = GetFileNameWithPath(szFilePath);
	// 去ext
	size_t nExtPos = tstrName.find_last_of(_T('.'));
	if (TSTRING::npos != nExtPos){
		tstrName.erase(nExtPos);
	}
	return tstrName;
}

// 获取没有后缀的文件名(去除目录，如：输入c:\abc.txt，输出abc)
inline TString GetFileNameExt(const TCHAR *szFilePath)
{
	TString tstrExt = szFilePath;
	// 去ext
	size_t nExtPos = tstrExt.find_last_of(_T('.'));
	if (TSTRING::npos != nExtPos){
		tstrExt.erase(0, nExtPos+1);
	}
	else {
		tstrExt = _T("");
	}
	return tstrExt;
}