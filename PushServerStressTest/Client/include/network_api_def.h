// network_api_def.h
//
#ifndef _INC_NETWORK_API_DEF_
#define _INC_NETWORK_API_DEF_

#include <string>

#ifndef STRSWITCH
#define STRSWITCH
#include <string>
#define SSWITCH(s) std::string strswitch_var = s; bool bstrswitch_found = false;
#define SCASE(s)   if (!bstrswitch_found && (strswitch_var == s ? (bstrswitch_found = true) : false))
#define SDEFAULT() if (!bstrswitch_found)
#endif//STRSWITCH


enum EGenEvent
{
	E_GENEVENT_UNKNOWN = 0,
	E_GENEVENT_INIT_OK,
	E_GENEVENT_INIT_ERROR,
	E_GENEVENT_SETREPORT_OK,
	E_GENEVENT_SETREPORT_ERROR,
};
enum ETelEvent
{
	E_TELEVENT_UNKNOWN = 10000,
	E_TELEVENT_INIT_OK, 
	E_TELEVENT_INIT_ERROR,
	E_TELEVENT_CALL_OK,
	E_TELEVENT_CALL_ERROR,	
	E_TELEVENT_CALL_CALLING_CORRECT,
	E_TELEVENT_CALL_CALLING_WRONG,
	E_TELEVENT_CALL_CONNING,
	E_TELEVENT_CALL_CONNTED,
	E_TELEVENT_CALL_DISCONNTED,
	E_TELEVENT_RING_RINGCOME,
	E_TELEVENT_RING_TIMER,
	E_TELEVENT_RING_DISCONNTED,
	E_TELEVENT_DROP_OK,
	E_TELEVENT_DROP_ERROR,
	E_TELEVENT_CALL_BUSY,
	E_TELEVENT_CALL_NOANSWER,
	
};

enum ESmsEvent
{
	E_SMSEVENT_UNKNOWN = 20000,
	E_SMSEVENT_INIT_OK, 
	E_SMSEVENT_INIT_ERROR,
	E_SMSEVENT_SETFORMAT_OK,
	E_SMSEVENT_SETFORMAT_ERROR,	
	E_SMSEVENT_SEND_ERROR,
	E_SMSEVENT_SEND_READY,
	E_SMSEVENT_SEND_OK,	
	E_SMSEVENT_RECV_OK,
	E_SMSEVENT_RECV_ERROR,
	E_SMSEVENT_DEL_OK,
	E_SMSEVENT_DEL_ERROR,

};

template <typename T> 
struct TypeName
{
	static std::string Name()
	{
		return typeid(T).name();
	}
};

struct NetworkEventParam
{
	virtual std::string ParamName()
	{
		return typeid(*this).name();
	}
};	

struct RingEventRingCome : public NetworkEventParam, public TypeName<RingEventRingCome>
{
	std::string sRingPhoneNum;
};


#endif//_INC_NETWORK_API_DEF_