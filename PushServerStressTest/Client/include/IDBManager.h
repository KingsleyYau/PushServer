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
/// 通知结构体
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
/// 消息结构体
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

// 通知列表
typedef std::list<NoticeInfo> NoticeInfoList;
// 消息列表
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

	virtual BOOL Init(const TCHAR *szDBFile, const TCHAR *szKey) = 0;                 // 初始化，数据库文件若存在则打开，否则创建数据库并建立上述中所有的表
	virtual BOOL AddNotice(const NoticeInfo& notice) = 0;                 // 添加通知到数据库
	virtual BOOL DelNotice(int nId) = 0;                                  // 删除一项通知
	virtual BOOL DelNotice(const NoticeInfoList& niList) = 0;             // 批量删除通知
	 
	virtual BOOL AddMessage(const MessageInfo& message) = 0;              // 添加消息到数据库
	virtual BOOL DelMessage(int nId) = 0;                                 // 删除消息
	virtual BOOL DelMessage(const MessageInfoList& miList) = 0;           // 批量删除消息

	// 获取每页为nItems条记录中第nPage页的通知列表
	virtual const NoticeInfoList GetNotices(int nPage, int nItems = -1, BOOL bSequence = FALSE) = 0;  
	// 获取每页为nItems条记录中第nPage页的消息列表
	virtual const MessageInfoList GetMessages(int nPage, int nItems = -1, BOOL bSequence = FALSE) = 0; 
};
