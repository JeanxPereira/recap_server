#ifndef __SIGNALED_EVENT_H
#define __SIGNALED_EVENT_H



#if   defined(_WIN32)
#include <windows.h>
#else
	#include <pthread.h>
	#include <sys/types.h>
	#include "SimpleMutex.h"




#endif

#include "Export.h"

class RAK_DLL_EXPORT SignaledEvent
{
public:
	SignaledEvent();
	~SignaledEvent();

	void InitEvent(void);
	void CloseEvent(void);
	void SetEvent(void);
	void WaitOnEvent(int timeoutMs);

protected:
#ifdef _WIN32
	HANDLE eventList;
#else
	SimpleMutex isSignaledMutex;
	bool isSignaled;
#if !defined(ANDROID)
	pthread_condattr_t condAttr;
#endif
	pthread_cond_t eventList;
	pthread_mutex_t hMutex;
	pthread_mutexattr_t mutexAttr;
#endif
};

#endif
