
#pragma once

#ifdef _COMMONUITL_BUILD_DLL

    #define COMMONUTIL_EXPORT	__declspec(dllexport)

#else

    #define COMMONUTIL_EXPORT

//#pragma comment(lib, "CommonUtil.lib") 

#endif
