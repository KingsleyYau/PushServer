// network_api_export.h
//
#ifndef _INC_NETWORK_API_EXPORT_
#define _INC_NETWORK_API_EXPORT_

#ifndef DllExport
	#ifdef _USRDLL
		#define DllExport __declspec(dllexport)
	#else
		#define DllExport
	#endif
#endif

#include <typeinfo.h>
#include <string>

class INetworkDevice;
class ISmsListener;
class ISms;
class ITel;

struct NetworkEventParam;

class INetworkDevice
{
public:
	virtual bool Init(unsigned int iComPort, unsigned int iComBaud=0) = 0;
	virtual bool Uninit() = 0;
	virtual ISms* GetSmsObject() = 0;
	virtual ITel* GetTelObject() = 0;
	virtual void Release() = 0;
};

class ISmsListener
{
public:
	virtual void OnError(int iErr) = 0;
	virtual void OnEvent(int iEvent, NetworkEventParam* pParam = NULL) = 0;
	virtual void OnRead(int iCount) = 0;
	virtual void OnSent(const char* cID, const char* cContent, const char* cPhone, BOOL bOK)=0;
};

class ISms
{
public:
	virtual bool AddListener(ISmsListener* pListener) = 0;
	virtual bool RmvListener(ISmsListener* pListener) = 0;
	virtual bool Send(const char* cID, const char* cContent, const char* cPhone, bool bNational = false) = 0;
	virtual bool Read(char* cContent, char* cPhone) = 0;
};	

class ITelListener
{
public:
	virtual void OnError(int iErr) = 0;
	virtual void OnEvent(int iEvent, NetworkEventParam* pParam = NULL) = 0;
};

class ITel
{
public:
	virtual bool AddListener(ITelListener* pListener) = 0;
	virtual bool RmvListener(ITelListener* pListener) = 0;
	virtual bool IsCalling(char* cPhone = NULL, unsigned int* piSize = NULL) = 0;
	virtual bool IsRinging(char* cPhone = NULL, unsigned int* piSize = NULL) = 0;
	virtual bool Call(const char* cPhone) = 0;
	virtual bool Drop() = 0;
};

#endif//_INC_NETWORK_API_EXPORT_