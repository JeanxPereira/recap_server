#include "RakThread.h"
#include "RakAssert.h"
#include "RakNetDefines.h"
#include "RakSleep.h"




#if   defined(_WIN32)
#include "WindowsIncludes.h"
#include <stdio.h>
	#if !defined(_WIN32_WCE)
	#include <process.h>
	#endif
#else
#include <pthread.h>
#endif

using namespace RakNet;

#if defined(_WIN32_WCE)
int RakThread::Create( LPTHREAD_START_ROUTINE start_address, void *arglist, int priority)
#elif defined(_WIN32)
int RakThread::Create( unsigned __stdcall start_address( void* ), void *arglist, int priority)
#else
int RakThread::Create( void* start_address( void* ), void *arglist, int priority)
#endif
{
#ifdef _WIN32
	HANDLE threadHandle;
	unsigned threadID = 0;


#if   defined (_WIN32_WCE)
	threadHandle = CreateThread(NULL,MAX_ALLOCA_STACK_ALLOCATION*2,start_address,arglist,0,(DWORD*)&threadID);
	SetThreadPriority(threadHandle, priority);
#else
	threadHandle = (HANDLE) _beginthreadex( NULL, MAX_ALLOCA_STACK_ALLOCATION*2, start_address, arglist, 0, &threadID );
#endif
	SetThreadPriority(threadHandle, priority);

	if (threadHandle==0)
	{
		return 1;
	}
	else
	{
		CloseHandle( threadHandle );
		return 0;
	}
#else
	pthread_t threadHandle;
	// Create thread linux
	pthread_attr_t attr;
	sched_param param;
	param.sched_priority=priority;
	pthread_attr_init( &attr );
	pthread_attr_setschedparam(&attr, &param);





	pthread_attr_setstacksize(&attr, MAX_ALLOCA_STACK_ALLOCATION*2);

	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	int res = pthread_create( &threadHandle, &attr, start_address, arglist );
	RakAssert(res==0 && "pthread_create in RakThread.cpp failed.")
	return res;
#endif
}
