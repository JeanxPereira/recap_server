#if defined(__X360__) 
#elif defined (_WIN32)
#include <WinSock2.h>
#include <windows.h>

// Must always include Winsock2.h before windows.h
// or else:
// winsock2.h(99) : error C2011: 'fd_set' : 'struct' type redefinition
// winsock2.h(134) : warning C4005: 'FD_SET' : macro redefinition
// winsock.h(83) : see previous definition of 'FD_SET'
// winsock2.h(143) : error C2011: 'timeval' : 'struct' type redefinition
// winsock2.h(199) : error C2011: 'hostent' : 'struct' type redefinition
// winsock2.h(212) : error C2011: 'netent' : 'struct' type redefinition
// winsock2.h(219) : error C2011: 'servent' : 'struct' type redefinition 

#endif
