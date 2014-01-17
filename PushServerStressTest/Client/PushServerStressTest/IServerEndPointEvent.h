#pragma once 
//IServerEndPoint
class IServerEndPointEvent
{
	void OnStablilshConnect(CString strRetMsg) = 0;
	void OnGetChallenge(CString strRetMsg) = 0;
	void OnPushMsg(CString strRetMsg) = 0;
};