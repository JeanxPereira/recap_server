/// \file
/// \brief All the message identifiers used by RakNet.  Message identifiers comprise the first byte of any message.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __MESSAGE_IDENTIFIERS_H
#define __MESSAGE_IDENTIFIERS_H 

#if defined(RAKNET_USE_CUSTOM_PACKET_IDS)
#include "CustomPacketIdentifiers.h"
#else

enum OutOfBandIdentifiers
{
	ID_NAT_ESTABLISH_UNIDIRECTIONAL,
	ID_NAT_ESTABLISH_BIDIRECTIONAL,
	ID_NAT_TYPE_DETECT,
	ID_ROUTER_2_REPLY_TO_SENDER_PORT,
	ID_ROUTER_2_REPLY_TO_SPECIFIED_PORT,
	ID_ROUTER_2_MINI_PUNCH_REPLY,
	ID_ROUTER_2_MINI_PUNCH_REPLY_BOUNCE
};

/// You should not edit the file MessageIdentifiers.h as it is a part of RakNet static library
/// To define your own message id, define an enum following the code example that follows. 
///
/// \code
/// enum {
///   ID_MYPROJECT_MSG_1 = ID_USER_PACKET_ENUM,
///   ID_MYPROJECT_MSG_2, 
///    ... 
/// };
/// \endcode 
///
/// \note All these enumerations should be casted to (unsigned char) before writing them to RakNet::BitStream
enum DefaultMessageIDTypes
{
	//
	// RESERVED TYPES - DO NOT CHANGE THESE
	// All types from RakPeer
	//
	/// These types are never returned to the user.
	/// Ping from a connected system.  Update timestamps (internal use only)
	/// 0 is reserved for UDT's connect message
	ID_INTERNAL_PING,  
	/// Ping from an unconnected system.  Reply but do not update timestamps. (internal use only)
	ID_PING,
	/// Ping from an unconnected system.  Only reply if we have open connections. Do not update timestamps. (internal use only)
	ID_PING_OPEN_CONNECTIONS,
	/// Pong from a connected system.  Update timestamps (internal use only)
	ID_CONNECTED_PONG,
	/// Asking for a new connection (internal use only)
	ID_CONNECTION_REQUEST,
	/// Connecting to a secured server/peer (internal use only)
	ID_SECURED_CONNECTION_RESPONSE,
	/// Connecting to a secured server/peer (internal use only)
	ID_SECURED_CONNECTION_CONFIRMATION,
	/// Packet that tells us the packet contains an integer ID to name mapping for the remote system (internal use only)
	ID_RPC_MAPPING,
	/// A reliable packet to detect lost connections (internal use only)
	ID_DETECT_LOST_CONNECTIONS,
	/// Offline message so we know when to reset and start a new connection (internal use only)
	ID_OPEN_CONNECTION_REQUEST,
	/// Offline message response so we know when to reset and start a new connection (internal use only)
	ID_OPEN_CONNECTION_REPLY,
	/// Remote procedure call (internal use only)
	ID_RPC,
	/// Remote procedure call reply, for RPCs that return data (internal use only)
	ID_RPC_REPLY,
	/// RakPeer - Same as ID_ADVERTISE_SYSTEM, but intended for internal use rather than being passed to the user. Second byte indicates type. Used currently for NAT punchthrough for receiver port advertisement. See ID_NAT_ADVERTISE_RECIPIENT_PORT
	ID_OUT_OF_BAND_INTERNAL,
	

	//
	// USER TYPES - DO NOT CHANGE THESE
	//

	/// RakPeer - In a client/server environment, our connection request to the server has been accepted.
	ID_CONNECTION_REQUEST_ACCEPTED,
	/// RakPeer - Sent to the player when a connection request cannot be completed due to inability to connect. 
	ID_CONNECTION_ATTEMPT_FAILED,
	/// RakPeer - Sent a connect request to a system we are currently connected to.
	ID_ALREADY_CONNECTED,
	/// RakPeer - A remote system has successfully connected.
	ID_NEW_INCOMING_CONNECTION,
	/// RakPeer - The system we attempted to connect to is not accepting new connections.
	ID_NO_FREE_INCOMING_CONNECTIONS,
	/// RakPeer - The system specified in Packet::systemAddress has disconnected from us.  For the client, this would mean the server has shutdown. 
	ID_DISCONNECTION_NOTIFICATION,
	/// RakPeer - Reliable packets cannot be delivered to the system specified in Packet::systemAddress.  The connection to that system has been closed. 
	ID_CONNECTION_LOST,
	/// RakPeer - We preset an RSA public key which does not match what the system we connected to is using.
	ID_RSA_PUBLIC_KEY_MISMATCH,
	/// RakPeer - We are banned from the system we attempted to connect to.
	ID_CONNECTION_BANNED,
	/// RakPeer - The remote system is using a password and has refused our connection because we did not set the correct password.
	ID_INVALID_PASSWORD,
	// RAKNET_PROTOCOL_VERSION in RakNetVersion.h does not match on the remote system what we have on our system
	// This means the two systems cannot communicate.
	// The 2nd byte of the message contains the value of RAKNET_PROTOCOL_VERSION for the remote system
	ID_INCOMPATIBLE_PROTOCOL_VERSION,
	// Means that this IP address connected recently, and can't connect again as a security measure. See RakPeer::SetLimitIPConnectionFrequency()
	ID_IP_RECENTLY_CONNECTED,
	/// RakPeer - A packet has been tampered with in transit.  The sender is contained in Packet::systemAddress.
	ID_MODIFIED_PACKET,
	/// RakPeer - The four bytes following this byte represent an unsigned int which is automatically modified by the difference in system times between the sender and the recipient. Requires that you call SetOccasionalPing.
	ID_TIMESTAMP,
    /// RakPeer - Pong from an unconnected system.  First byte is ID_PONG, second sizeof(RakNetTime) bytes is the ping, following bytes is system specific enumeration data.
	ID_PONG,
	/// RakPeer - Inform a remote system of our IP/Port. On the recipient, all data past ID_ADVERTISE_SYSTEM is whatever was passed to the data parameter
	ID_ADVERTISE_SYSTEM,
	/// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has disconnected gracefully.  Packet::systemAddress is modified to reflect the systemAddress of this client.
	ID_REMOTE_DISCONNECTION_NOTIFICATION,
	/// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has been forcefully dropped. Packet::systemAddress is modified to reflect the systemAddress of this client.
	ID_REMOTE_CONNECTION_LOST,
	/// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has connected.  Packet::systemAddress is modified to reflect the systemAddress of the client that is not connected directly to us. The packet encoding is SystemAddress 1, ConnectionGraphGroupID 1, SystemAddress 2, ConnectionGraphGroupID 2
	/// ConnectionGraph2 plugin: Bytes 1-4 = count. for (count items) contains {SystemAddress, RakNetGUID}
	ID_REMOTE_NEW_INCOMING_CONNECTION,
	// RakPeer - Downloading a large message. Format is ID_DOWNLOAD_PROGRESS (MessageID), partCount (unsigned int), partTotal (unsigned int), partLength (unsigned int), first part data (length <= MAX_MTU_SIZE). See the three parameters partCount, partTotal and partLength in OnFileProgress in FileListTransferCBInterface.h
	ID_DOWNLOAD_PROGRESS,
	
	/// FileListTransfer plugin - Setup data
	ID_FILE_LIST_TRANSFER_HEADER,
	/// FileListTransfer plugin - A file
	ID_FILE_LIST_TRANSFER_FILE,
	// Ack for reference push, to send more of the file
	ID_FILE_LIST_REFERENCE_PUSH_ACK,

	/// DirectoryDeltaTransfer plugin - Request from a remote system for a download of a directory
	ID_DDT_DOWNLOAD_REQUEST,
	
	/// RakNetTransport plugin - Transport provider message, used for remote console
	ID_TRANSPORT_STRING,

	/// ReplicaManager plugin - Create an object
	ID_REPLICA_MANAGER_CONSTRUCTION,
	/// ReplicaManager plugin - Destroy an object
	ID_REPLICA_MANAGER_DESTRUCTION,
	/// ReplicaManager plugin - Changed scope of an object
	ID_REPLICA_MANAGER_SCOPE_CHANGE,
	/// ReplicaManager plugin - Serialized data of an object
	ID_REPLICA_MANAGER_SERIALIZE,
	/// ReplicaManager plugin - New connection, about to send all world objects
	ID_REPLICA_MANAGER_DOWNLOAD_STARTED,
	/// ReplicaManager plugin - Finished downloading all serialized objects
	ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE,

	/// ConnectionGraph plugin - Request the connection graph from another system
	ID_CONNECTION_GRAPH_REQUEST,
	/// ConnectionGraph plugin - Reply to a connection graph download request
	ID_CONNECTION_GRAPH_REPLY,
	/// ConnectionGraph plugin - Update edges / nodes for a system with a connection graph
	ID_CONNECTION_GRAPH_UPDATE,
	/// ConnectionGraph plugin - Add a new connection to a connection graph
	ID_CONNECTION_GRAPH_NEW_CONNECTION,
	/// ConnectionGraph plugin - Remove a connection from a connection graph - connection was abruptly lost. Two systems addresses encoded in the data packet.
	ID_CONNECTION_GRAPH_CONNECTION_LOST,
	/// ConnectionGraph plugin - Remove a connection from a connection graph - connection was gracefully lost. Two systems addresses encoded in the data packet.
	ID_CONNECTION_GRAPH_DISCONNECTION_NOTIFICATION,

	/// Router plugin - route a message through another system
	ID_ROUTE_AND_MULTICAST,

	/// RakVoice plugin - Open a communication channel
	ID_RAKVOICE_OPEN_CHANNEL_REQUEST,
	/// RakVoice plugin - Communication channel accepted
	ID_RAKVOICE_OPEN_CHANNEL_REPLY,
	/// RakVoice plugin - Close a communication channel
	ID_RAKVOICE_CLOSE_CHANNEL,
	/// RakVoice plugin - Voice data
	ID_RAKVOICE_DATA,

	/// Autopatcher plugin - Get a list of files that have changed since a certain date
	ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE,
	/// Autopatcher plugin - A list of files to create
	ID_AUTOPATCHER_CREATION_LIST,
	/// Autopatcher plugin - A list of files to delete
	ID_AUTOPATCHER_DELETION_LIST,
	/// Autopatcher plugin - A list of files to get patches for
	ID_AUTOPATCHER_GET_PATCH,
	/// Autopatcher plugin - A list of patches for a list of files
	ID_AUTOPATCHER_PATCH_LIST,
	/// Autopatcher plugin - Returned to the user: An error from the database repository for the autopatcher.
	ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR,
	/// Autopatcher plugin - Finished getting all files from the autopatcher
	ID_AUTOPATCHER_FINISHED_INTERNAL,
	ID_AUTOPATCHER_FINISHED,
	/// Autopatcher plugin - Returned to the user: You must restart the application to finish patching.
	ID_AUTOPATCHER_RESTART_APPLICATION,

	/// NATPunchthrough plugin: internal
	ID_NAT_PUNCHTHROUGH_REQUEST,
	/// NATPunchthrough plugin: internal
	ID_NAT_CONNECT_AT_TIME,
	/// NATPunchthrough plugin: internal
	ID_NAT_GET_MOST_RECENT_PORT,
	/// NATPunchthrough plugin: internal
	ID_NAT_CLIENT_READY,

	/// NATPunchthrough plugin: Destination system is not connected to the server. Bytes starting at offset 1 contains the RakNetGUID destination field of NatPunchthroughClient::OpenNAT().
	ID_NAT_TARGET_NOT_CONNECTED,
	/// NATPunchthrough plugin: Destination system is not responding to the plugin messages. Possibly the plugin is not installed. Bytes starting at offset 1 contains the RakNetGUID  destination field of NatPunchthroughClient::OpenNAT().
	ID_NAT_TARGET_UNRESPONSIVE,
	/// NATPunchthrough plugin: The server lost the connection to the destination system while setting up punchthrough. Possibly the plugin is not installed. Bytes starting at offset 1 contains the RakNetGUID  destination field of NatPunchthroughClient::OpenNAT().
	ID_NAT_CONNECTION_TO_TARGET_LOST,
	/// NATPunchthrough plugin: This punchthrough is already in progress. Possibly the plugin is not installed. Bytes starting at offset 1 contains the RakNetGUID destination field of NatPunchthroughClient::OpenNAT().
	ID_NAT_ALREADY_IN_PROGRESS,
	/// NATPunchthrough plugin: This message is generated on the local system, and does not come from the network. packet::guid contains the destination field of NatPunchthroughClient::OpenNAT(). Byte 1 contains 1 if you are the sender, 0 if not
	ID_NAT_PUNCHTHROUGH_FAILED,
	/// NATPunchthrough plugin: Punchthrough suceeded. See packet::systemAddress and packet::guid. Byte 1 contains 1 if you are the sender, 0 if not. You can now use RakPeer::Connect() or other calls to communicate with this system.
	ID_NAT_PUNCHTHROUGH_SUCCEEDED,

	/// LightweightDatabase plugin - Query
	ID_DATABASE_QUERY_REQUEST,
	/// LightweightDatabase plugin - Update
	ID_DATABASE_UPDATE_ROW,
	/// LightweightDatabase plugin - Remove
	ID_DATABASE_REMOVE_ROW,
	/// LightweightDatabase plugin - A serialized table.  Bytes 1+ contain the table.  Pass to TableSerializer::DeserializeTable
	ID_DATABASE_QUERY_REPLY,
	/// LightweightDatabase plugin - Specified table not found
	ID_DATABASE_UNKNOWN_TABLE,
	/// LightweightDatabase plugin - Incorrect password
	ID_DATABASE_INCORRECT_PASSWORD,

	/// ReadyEvent plugin - Set the ready state for a particular system
	/// First 4 bytes after the message contains the id
	ID_READY_EVENT_SET,
	/// ReadyEvent plugin - Unset the ready state for a particular system
	/// First 4 bytes after the message contains the id
	ID_READY_EVENT_UNSET,
	/// All systems are in state ID_READY_EVENT_SET
	/// First 4 bytes after the message contains the id
	ID_READY_EVENT_ALL_SET,
	/// \internal, do not process in your game
	/// ReadyEvent plugin - Request of ready event state - used for pulling data when newly connecting
	ID_READY_EVENT_QUERY,

	/// Lobby packets. Second byte indicates type.
	ID_LOBBY_GENERAL,

	/// Auto RPC procedure call
	ID_AUTO_RPC_CALL,

	/// Auto RPC functionName to index mapping
	ID_AUTO_RPC_REMOTE_INDEX,

	/// Auto RPC functionName to index mapping, lookup failed. Will try to auto recover
	ID_AUTO_RPC_UNKNOWN_REMOTE_INDEX,

	/// Auto RPC error code
	/// See AutoRPC.h for codes, stored in packet->data[1]
	ID_RPC_REMOTE_ERROR,

	/// FileListTransfer transferring large files in chunks that are read only when needed, to save memory
	ID_FILE_LIST_REFERENCE_PUSH,

	/// Force the ready event to all set
	ID_READY_EVENT_FORCE_ALL_SET,

	/// Rooms function
	ID_ROOMS_EXECUTE_FUNC,
	ID_ROOMS_LOGON_STATUS,
	ID_ROOMS_HANDLE_CHANGE,

	/// Lobby2 message
	ID_LOBBY2_SEND_MESSAGE,
	ID_LOBBY2_SERVER_ERROR,


	/// Informs user of a new host GUID. Packet::Guid contains this RakNetGuid
	ID_FCM2_NEW_HOST,
	/// \internal For FullyConnectedMesh2 plugin
	ID_FCM2_REQUEST_FCMGUID,
	/// \internal For FullyConnectedMesh2 plugin
	ID_FCM2_RESPOND_CONNECTION_COUNT,
	/// \internal For FullyConnectedMesh2 plugin
	ID_FCM2_INFORM_FCMGUID,

	/// UDP proxy messages. Second byte indicates type.
	ID_UDP_PROXY_GENERAL,

	/// SQLite3Plugin - execute
	ID_SQLite3_EXEC,
	/// SQLite3Plugin - Remote database is unknown
	ID_SQLite3_UNKNOWN_DB,

	/// Serialize construction for an object that already exists on the remote system
	ID_REPLICA_MANAGER_3_SERIALIZE_CONSTRUCTION_EXISTING,
	ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_REJECTED,
	ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_ACCEPTED,

	/// Sent to NatTypeDetectionServer
	ID_NAT_TYPE_DETECTION_REQUEST,

	/// Sent to NatTypeDetectionClient. Byte 1 contains the type of NAT detected.
	ID_NAT_TYPE_DETECTION_RESULT,

	/// Events happening with SQLiteClientLoggerPlugin
	ID_SQLLITE_LOGGER,

	/// Used by the router2 plugin
	ID_ROUTER_2_INTERNAL,
	/// No path is available or can be established to the remote system
	/// Packet::guid contains the endpoint guid that we were trying to reach
	ID_ROUTER_2_FORWARDING_NO_PATH,
	/// \brief You can now call connect, ping, or other operations to the destination system.
	///
	/// Connect as follows:
	///
	/// RakNet::BitStream bs(packet->data, packet->length, false);
	/// bs.IgnoreBytes(sizeof(MessageID));
	/// RakNetGUID endpointGuid;
	/// bs.Read(endpointGuid);
	/// unsigned short sourceToDestPort;
	/// bs.Read(sourceToDestPort);
	/// char ipAddressString[32];
	/// packet->systemAddress.ToString(false, ipAddressString);
	/// rakPeerInterface->Connect(ipAddressString, sourceToDestPort, 0,0);
	ID_ROUTER_2_FORWARDING_ESTABLISHED,
	/// The IP address for a forwarded connection has changed
	/// Read endpointGuid and port as per ID_ROUTER_2_FORWARDING_ESTABLISHED
	ID_ROUTER_2_REROUTED,

	/// \internal Used by the team balancer plugin
	ID_TEAM_BALANCER_INTERNAL,
	/// Cannot switch to the desired team because it is full. However, if someone on that team leaves, you will get ID_TEAM_BALANCER_SET_TEAM later. Byte 1 contains the team you requested to join.
	ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING,
	/// Cannot switch to the desired team because all teams are locked. However, if someone on that team leaves, you will get ID_TEAM_BALANCER_SET_TEAM later. Byte 1 contains the team you requested to join.
	ID_TEAM_BALANCER_TEAMS_LOCKED,
	/// Team balancer plugin informing you of your team. Byte 1 contains the team you requested to join.
	ID_TEAM_BALANCER_TEAM_ASSIGNED,
	/// Gamebryo Lightspeed
	ID_LIGHTSPEED_INTEGRATION,

	/// Plugin based replacement for old RPC system, no boost required, but only works with C functions
	ID_RPC_4_PLUGIN,

	/// If RakPeerInterface::Send() is called where PacketReliability contains _WITH_ACK_RECEIPT, then on a later call to RakPeerInterface::Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS. The message will be 5 bytes long, and bytes 1-4 inclusive will contain a number in native order containing a number that identifies this message. This number will be returned by RakPeerInterface::Send() or RakPeerInterface::SendList(). ID_SND_RECEIPT_ACKED means that the message arrived
	ID_SND_RECEIPT_ACKED,

	/// If RakPeerInterface::Send() is called where PacketReliability contains _WITH_ACK_RECEIPT, then on a later call to RakPeerInterface::Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS. The message will be 5 bytes long, and bytes 1-4 inclusive will contain a number in native order containing a number that identifies this message. This number will be returned by RakPeerInterface::Send() or RakPeerInterface::SendList(). ID_SND_RECEIPT_LOSS means that an ack for the message did not arrive (it may or may not have been delivered, probably not). On disconnect or shutdown, you will not get ID_SND_RECEIPT_LOSS for unsent messages, you should consider those messages as all lost.
	ID_SND_RECEIPT_LOSS,

	// So I can add more without changing user enumerations
	ID_RESERVED_5,
	ID_RESERVED_6,
	ID_RESERVED_7,
	ID_RESERVED_8,
	ID_RESERVED_9,

	// For the user to use.  Start your first enumeration at this value.
	ID_USER_PACKET_ENUM,
	//-------------------------------------------------------------------------------------------------------------
 
};

#endif // RAKNET_USE_CUSTOM_PACKET_IDS

#endif
