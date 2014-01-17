/*
 * File         : IDBManager.cpp
 * Data         : 2011-06-01
 * Author       : Heng Qiu
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : drcom client controler
 */

#pragma once

#include <windows.h>
#include <type.h>
#include <tchar.h>
#include <string> 
#include <list>
using namespace std;

///////////////
/// ֪ͨ�ṹ��
///////////////
typedef struct _tagNoticeInfo
{
	int nId;
	TSTRING tsTime;
	TSTRING tsFrom;
	TSTRING tsTo;
	TSTRING tsTitle;
	TSTRING tsContent;
	TSTRING tsUrl;
}NoticeInfo, *LPNoticeInfo;

///////////////
/// ��Ϣ�ṹ��
///////////////
typedef struct _tagMessageInfo
{
	int nId;
	TSTRING tsTime;
	TSTRING tsFrom;
	TSTRING tsTo;
	TSTRING tsTitle;
	TSTRING tsContent;
	TSTRING tsUrl;
}MessageInfo, *LPMessageInfo;

// ֪ͨ�б�
typedef std::list<NoticeInfo> NoticeInfoList;
// ��Ϣ�б�
typedef std::list<MessageInfo> MessageInfoList;


class IDBManager
{
	// Contruction
protected:
	IDBManager(void);
	virtual ~IDBManager(void);

	// Method
public:
	static IDBManager* Create();
	static void Release(IDBManager *obj);

	virtual BOOL Init(const TCHAR *szDBFile, const TCHAR *szKey) = 0;                 // ��ʼ�������ݿ��ļ���������򿪣����򴴽����ݿⲢ�������������еı�
	virtual BOOL AddNotice(const NoticeInfo& notice) = 0;                 // ���֪ͨ�����ݿ�
	virtual BOOL DelNotice(int nId) = 0;                                  // ɾ��һ��֪ͨ
	virtual BOOL DelNotice(const NoticeInfoList& niList) = 0;             // ����ɾ��֪ͨ
	 
	virtual BOOL AddMessage(const MessageInfo& message) = 0;              // �����Ϣ�����ݿ�
	virtual BOOL DelMessage(int nId) = 0;                                 // ɾ����Ϣ
	virtual BOOL DelMessage(const MessageInfoList& miList) = 0;           // ����ɾ����Ϣ

	// ��ȡÿҳΪnItems����¼�е�nPageҳ��֪ͨ�б�
	virtual const NoticeInfoList GetNotices(int nPage, int nItems = -1, BOOL bSequence = FALSE) = 0;  
	// ��ȡÿҳΪnItems����¼�е�nPageҳ����Ϣ�б�
	virtual const MessageInfoList GetMessages(int nPage, int nItems = -1, BOOL bSequence = FALSE) = 0; 
};
