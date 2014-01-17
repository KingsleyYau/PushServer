/*
 * File         : PathProc.h
 * Date         : 2011-06-07
 * Author       : FGX
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : ·������
 */

#pragma once
#include <string>
#include <windows.h>

#define TString std::string
#define TSTRING TString

// ��ȡ��ǰĿ¼·��
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
			// .\��ͷ
			if(tsRelative.at(1) == '\\')
			{
				tsTemp = tsRelative.substr(2,tsCurrPath.length() - 2);
				tsResult += tsTemp;
				break;
			}
			// ./��ͷ
			else if(tsRelative.at(1) == '/')
			{
				tsTemp = tsRelative.substr(2,tsCurrPath.length() - 2);
				tsResult += tsTemp;
				break;
			}
			// ..\��ͷ
			else	
				tsResult += tsRelative;/* ��ת��������� */
		}break;
	case '\\':  // \��ͷ
		{
			tsTemp = tsRelative.substr(1,tsCurrPath.length() - 1);
			tsResult += tsTemp;
			break;
		}break;
	default:   // ������� a* 
		{
			tsResult += tsRelative;
		}break;
	}

	return tsResult;
}

// �������·����ȡ����·��
inline TString GetAbsolutePath(const TCHAR* cRelativePath)
{
	return GetAbsolutePath(TString(cRelativePath));
}

// ��ȡ�ļ���(ȥ��Ŀ¼���磺����c:\abc.txt�����abc.txt)
inline TString GetFileNameWithPath(const TCHAR *szFilePath)
{
	TString tstrName = szFilePath;
	// ȥĿ¼
	size_t nDirPos = tstrName.find_last_of(_T('\\'));
	if (TString::npos != nDirPos){
		tstrName.erase(0, nDirPos+1);
	}
	return tstrName;
}

// ��ȡû�к�׺���ļ���(ȥ��Ŀ¼���磺����c:\abc.txt�����abc)
inline TString GetFileNameWithoutExt(const TCHAR *szFilePath)
{
	TString tstrName = GetFileNameWithPath(szFilePath);
	// ȥext
	size_t nExtPos = tstrName.find_last_of(_T('.'));
	if (TSTRING::npos != nExtPos){
		tstrName.erase(nExtPos);
	}
	return tstrName;
}

// ��ȡû�к�׺���ļ���(ȥ��Ŀ¼���磺����c:\abc.txt�����abc)
inline TString GetFileNameExt(const TCHAR *szFilePath)
{
	TString tstrExt = szFilePath;
	// ȥext
	size_t nExtPos = tstrExt.find_last_of(_T('.'));
	if (TSTRING::npos != nExtPos){
		tstrExt.erase(0, nExtPos+1);
	}
	else {
		tstrExt = _T("");
	}
	return tstrExt;
}