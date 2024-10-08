/// \file
/// \brief A simple TCP based server allowing sends and receives.  Can be connected by any TCP client, including telnet.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_TCPInterface==1

#ifndef __SIMPLE_TCP_SERVER
#define __SIMPLE_TCP_SERVER

#include "RakMemoryOverride.h"
#include "DS_List.h"
#include "RakNetTypes.h"
#include "Export.h"
#include "RakThread.h"
#include "DS_Queue.h"
#include "SimpleMutex.h"
#include "RakNetDefines.h"
#include "SocketIncludes.h"
#include "DS_ByteQueue.h"
#include "DS_ThreadsafeAllocatingQueue.h"

struct RemoteClient;

#if OPEN_SSL_CLIENT_SUPPORT==1
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

/// \internal
/// \brief As the name says, a simple multithreaded TCP server.  Used by TelnetTransport
class RAK_DLL_EXPORT TCPInterface
{
public:
	TCPInterface();
	virtual ~TCPInterface();

	/// Starts the TCP server on the indicated port
	/// \param[in] threadPriority Passed to the thread creation routine. Use THREAD_PRIORITY_NORMAL for Windows. For Linux based systems, you MUST pass something reasonable based on the thread priorities for your application.
	bool Start(unsigned short port, unsigned short maxIncomingConnections, unsigned short maxConnections=0, int _threadPriority=-99999);

	/// Stops the TCP server
	void Stop(void);

	/// Connect to the specified host on the specified port
	SystemAddress Connect(const char* host, unsigned short remotePort, bool block=true);

#if OPEN_SSL_CLIENT_SUPPORT==1
	/// Start SSL on an existing connection, notified with HasCompletedConnectionAttempt
	void StartSSLClient(SystemAddress systemAddress);

	/// Was SSL started on this socket?
	bool IsSSLActive(SystemAddress systemAddress);
#endif

	/// Sends a byte stream
	void Send( const char *data, unsigned int length, SystemAddress systemAddress, bool broadcast );

	// Sends a concatenated list of byte streams
	bool SendList( const char **data, const unsigned int  *lengths, const int numParameters, SystemAddress systemAddress, bool broadcast );

	// Get how many bytes are waiting to be sent. If too many, you may want to skip sending
	unsigned int GetOutgoingDataBufferSize(SystemAddress systemAddress) const;

	/// Returns if Receive() will return data
	bool ReceiveHasPackets( void );

	/// Returns data received
	Packet* Receive( void );

	/// Disconnects a player/address
	void CloseConnection( SystemAddress systemAddress );

	/// Deallocates a packet returned by Receive
	void DeallocatePacket( Packet *packet );

	/// Fills the array remoteSystems with the SystemAddress of all the systems we are connected to
	/// \param[out] remoteSystems An array of SystemAddress structures to be filled with the SystemAddresss of the systems we are connected to. Pass 0 to remoteSystems to only get the number of systems we are connected to
	/// \param[in, out] numberOfSystems As input, the size of remoteSystems array.  As output, the number of elements put into the array 
	void GetConnectionList( SystemAddress *remoteSystems, unsigned short *numberOfSystems ) const;

	/// Returns just the number of connections we have
	unsigned short GetConnectionCount(void) const;

	/// Has a previous call to connect succeeded?
	/// \return UNASSIGNED_SYSTEM_ADDRESS = no. Anything else means yes.
	SystemAddress HasCompletedConnectionAttempt(void);

	/// Has a previous call to connect failed?
	/// \return UNASSIGNED_SYSTEM_ADDRESS = no. Anything else means yes.
	SystemAddress HasFailedConnectionAttempt(void);

	/// Queued events of new incoming connections
	SystemAddress HasNewIncomingConnection(void);

	/// Queued events of lost connections
	SystemAddress HasLostConnection(void);

	/// Return an allocated but empty packet, for custom use
	Packet* AllocatePacket(unsigned dataSize);

	// Push a packet back to the queue
	virtual void PushBackPacket( Packet *packet, bool pushAtHead );
protected:

	bool isStarted, threadRunning;
	SOCKET listenSocket;

	DataStructures::Queue<Packet*> headPush, tailPush;
	RemoteClient* remoteClients;
	int remoteClientsLength;

	// Assuming remoteClients is only used by one thread!
	// DataStructures::List<RemoteClient*> remoteClients;
	// Use this thread-safe queue to add to remoteClients
	// DataStructures::Queue<RemoteClient*> remoteClientsInsertionQueue;
	// SimpleMutex remoteClientsInsertionQueueMutex;

	/*
	struct OutgoingMessage
	{
		unsigned char* data;
		SystemAddress systemAddress;
		bool broadcast;
		unsigned int length;
	};
	*/
//	DataStructures::SingleProducerConsumer<OutgoingMessage> outgoingMessages;
//	DataStructures::SingleProducerConsumer<Packet> incomingMessages;
//	DataStructures::SingleProducerConsumer<SystemAddress> newIncomingConnections, lostConnections, requestedCloseConnections;
//	DataStructures::SingleProducerConsumer<RemoteClient*> newRemoteClients;
//	DataStructures::ThreadsafeAllocatingQueue<OutgoingMessage> outgoingMessages;
	DataStructures::ThreadsafeAllocatingQueue<Packet> incomingMessages;
	DataStructures::ThreadsafeAllocatingQueue<SystemAddress> newIncomingConnections, lostConnections, requestedCloseConnections;
	DataStructures::ThreadsafeAllocatingQueue<RemoteClient*> newRemoteClients;
	SimpleMutex completedConnectionAttemptMutex, failedConnectionAttemptMutex;
	DataStructures::Queue<SystemAddress> completedConnectionAttempts, failedConnectionAttempts;

	int threadPriority;

	DataStructures::List<SOCKET> blockingSocketList;
	SimpleMutex blockingSocketListMutex;

	friend RAK_THREAD_DECLARATION(UpdateTCPInterfaceLoop);
	friend RAK_THREAD_DECLARATION(ConnectionAttemptLoop);

//	void DeleteRemoteClient(RemoteClient *remoteClient, fd_set *exceptionFD);
//	void InsertRemoteClient(RemoteClient* remoteClient);
	SOCKET SocketConnect(const char* host, unsigned short remotePort);

	struct ThisPtrPlusSysAddr
	{
		TCPInterface *tcpInterface;
		SystemAddress systemAddress;
		bool useSSL;
	};

#if OPEN_SSL_CLIENT_SUPPORT==1
	SSL_CTX* ctx;
	SSL_METHOD *meth;
	DataStructures::ThreadsafeAllocatingQueue<SystemAddress> startSSL;
	DataStructures::List<SystemAddress> activeSSLConnections;
	SimpleMutex sharedSslMutex;
#endif
};

/// Stores information about a remote client.
struct RemoteClient
{
	RemoteClient() {
#if OPEN_SSL_CLIENT_SUPPORT==1
		ssl=0;
#endif
		isActive=false;
		socket=INVALID_SOCKET;
	}
	SOCKET socket;
	SystemAddress systemAddress;
	DataStructures::ByteQueue outgoingData;
	bool isActive;
	SimpleMutex outgoingDataMutex;
	SimpleMutex isActiveMutex;

#if OPEN_SSL_CLIENT_SUPPORT==1
	SSL*     ssl;
	void InitSSL(SSL_CTX* ctx, SSL_METHOD *meth);
	void DisconnectSSL(void);
	void FreeSSL(void);
	int Send(const char *data, unsigned int length);
	int Recv(char *data, const int dataSize);
#else
	int Send(const char *data, unsigned int length);
	int Recv(char *data, const int dataSize);
#endif
	void Reset(void)
	{
		outgoingDataMutex.Lock();
		outgoingData.Clear(__FILE__,__LINE__);
		outgoingDataMutex.Unlock();
	}
	void SetActive(bool a);
	void SendOrBuffer(const char **data, const unsigned int *lengths, const int numParameters);
};

#endif

#endif // _RAKNET_SUPPORT_*

