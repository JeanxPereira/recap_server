
// Include
#include "functions.h"
#include "utils/functions.h"

// Blaze
namespace Blaze {
	// ClientData
	void ClientData::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			serviceName = value["SVCN"].GetString();

			lang = value["LANG"].GetUint();
			type = static_cast<ClientType>(value["TYPE"].GetUint());

			iito = value["IITO"].GetUint() != 0;
		}
	}

	void ClientData::Write(TDF::Packet& packet) const {
		packet.put_integer("IITO", iito ? 1 : 0);
		packet.put_integer("LANG", lang);
		packet.put_string("SVCN", serviceName);
		packet.put_integer("TYPE", type);
	}

	// IpAddress
	void IpAddress::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			address = value["IP"].GetUint();
			port = static_cast<uint16_t>(value["PORT"].GetUint());
		}
	}

	void IpAddress::Write(TDF::Packet& packet) const {
		packet.put_integer("IP", address);
		packet.put_integer("PORT", port);
	}

	// HostNameAddress
	void HostNameAddress::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			name = value["NAME"].GetString();
			port = static_cast<uint16_t>(value["PORT"].GetUint());
		}
	}

	void HostNameAddress::Write(TDF::Packet& packet) const {
		packet.put_string("NAME", name);
		packet.put_integer("PORT", port);
	}

	// IpPairAddress
	void IpPairAddress::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			auto end = value.MemberEnd();
			auto exipIterator = value.FindMember("EXIP");
			if (exipIterator != end) {
				exip.Read(exipIterator->value);
			}

			auto inipIterator = value.FindMember("INIP");
			if (inipIterator != end) {
				inip.Read(inipIterator->value);
			}
		}
	}

	void IpPairAddress::Write(TDF::Packet& packet) const {
		packet.push_struct("EXIP");
		exip.Write(packet);
		packet.pop();

		packet.push_struct("INIP");
		inip.Write(packet);
		packet.pop();
	}

	// NetworkQosData
	void NetworkQosData::Write(TDF::Packet& packet) const {
		packet.put_integer("DBPS", dbps);
		packet.put_integer("NATT", type);
		packet.put_integer("UBPS", ubps);
	}

	// QosPingSiteInfo
	void QosPingSiteInfo::Write(TDF::Packet& packet) const {
		packet.put_string("PSA", address);
		packet.put_integer("PSP", port);
		packet.put_string("SNA", name);
	}

	// QosConfigInfo
	void QosConfigInfo::Write(TDF::Packet& packet) const {
		packet.push_struct("BWPS");
		if (!pingSiteInfoByAlias.empty()) {
			pingSiteInfoByAlias.begin()->second.Write(packet);
		}
		packet.pop();

		packet.put_integer("LNP", lnp);

		packet.push_map("LTPS", TDF::Type::String, TDF::Type::Struct);
		for (const auto& [name, siteInfo] : pingSiteInfoByAlias) {
			packet.push_struct(name);
			siteInfo.Write(packet);
			packet.pop();
		}
		packet.pop();

		packet.put_integer("SVID", svid);
	}

	// HostInfo
	void HostInfo::Write(TDF::Packet& packet) const {
		packet.put_integer("HPID", id);
		packet.put_integer("HSLT", slot);
	}

	// PersonaDetails
	void PersonaDetails::Write(TDF::Packet& packet) const {
		packet.put_string("DSNM", name);
		packet.put_integer("LAST", last);
		packet.put_integer("PID", id);
		packet.put_integer("STAS", status);
		packet.put_integer("XREF", externalReference);
		packet.put_integer("XTYP", externalReferenceType);
	}

	// SessionInfo
	void SessionInfo::Write(TDF::Packet& packet) const {
		packet.put_integer("BUID", blazeId);
		packet.put_integer("FRST", firstLogin ? 1 : 0);
		packet.put_string("KEY", key);
		packet.put_integer("LLOG", lastLogin);
		packet.put_string("MAIL", email);

		packet.push_struct("PDTL");
		personaDetails.Write(packet);
		packet.pop();

		packet.put_integer("UID", uid);
	}

	// ListIdentification
	void ListIdentification::Write(TDF::Packet& packet) const {
		packet.put_string("LNM", name);
		packet.put_integer("TYPE", type);
	}

	// ListInfo
	void ListInfo::Write(TDF::Packet& packet) const {
		packet.put_object_id("BOID", std::get<0>(blazeObjectId), std::get<1>(blazeObjectId), std::get<2>(blazeObjectId));
		packet.put_integer("FLGS", flags);

		packet.push_struct("LID");
		identification.Write(packet);
		packet.pop();

		packet.put_integer("LMS", lms);
		packet.put_integer("PRID", prid);
	}

	// ListMemberId
	void ListMemberId::Write(TDF::Packet& packet) const {
		packet.put_integer("BLID", id);
		packet.put_string("PNAM", name);
		packet.put_integer("XREF", externalReference);
		packet.put_integer("XTYP", externalReferenceType);
	}

	// ListMemberInfo
	void ListMemberInfo::Write(TDF::Packet& packet) const {
		packet.push_struct("LMID");
		id.Write(packet);
		packet.pop();

		packet.put_integer("TIME", time);
	}

	// ListMemberInfoUpdate
	void ListMemberInfoUpdate::Write(TDF::Packet& packet) const {
		packet.push_struct("LMID");
		info.Write(packet);
		packet.pop();

		packet.put_integer("LUPT", type);
	}

	// ClientMessage
	void ClientMessage::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			const auto& attrIterator = value.FindMember("ATTR");
			if (attrIterator != value.MemberEnd()) {
				for (const auto& attribute : attrIterator->value["_Content"].GetObject()) {
					attributes.emplace(utils::to_number<uint32_t>(attribute.name.GetString(), 0), attribute.value.GetString());
				}
			}

			const auto& targetData = value["TARG"];
			std::get<0>(target) = targetData["_X"].GetUint();
			std::get<1>(target) = targetData["_Y"].GetUint();
			std::get<2>(target) = targetData["_Z"].GetUint64();

			flags = value["FLAG"].GetUint();
			stat = value["STAT"].GetUint();
			tag = value["TAG"].GetUint();
			type = value["TYPE"].GetUint();
		}
	}

	void ClientMessage::Write(TDF::Packet& packet) const {
		packet.push_map("ATTR", TDF::Type::Integer, TDF::Type::String);
		for (const auto& [id, message] : attributes) {
			packet.put_string(std::to_string(id), message);
		}
		packet.pop();

		packet.put_integer("FLAG", flags);
		packet.put_integer("STAT", stat);
		packet.put_integer("TAG", tag);
		packet.put_object_id("TARG", target);
		packet.put_integer("TYPE", type);
	}

	// ServerMessage
	void ServerMessage::Write(TDF::Packet& packet) const {
		packet.put_integer("FLAG", flags);
		packet.put_integer("MGID", messageId);
		packet.put_string("NAME", name);

		packet.push_struct("PYLD");
		message.Write(packet);
		packet.pop();

		packet.put_object_id("SRCE", source);
		packet.put_integer("TIME", time);
	}

	// UserIdentification
	void UserIdentification::Write(TDF::Packet& packet) const {
		packet.put_integer("AID", id);
		packet.put_integer("ALOC", localization);
		packet.put_blob("EXBB", nullptr, 0); // blob, unused?
		packet.put_integer("EXID", 0); // uint64_t, unused?
		packet.put_integer("ID", id);
		packet.put_string("NAME", name);
	}

	// PlaygroupInfo
	void PlaygroupInfo::Write(TDF::Packet& packet) const {
		packet.push_map("ATTR", TDF::Type::Integer, TDF::Type::String);
		for (const auto& [key, value] : attributes) {
			packet.put_string(key, value);
		}
		packet.pop();

		packet.put_integer("ENBV", enbv ? 1 : 0);

		packet.push_union("HNET", NetworkAddressMember::Unset);
		// TODO: check if this is neccesary
		packet.pop();

		packet.put_integer("HSID", hostSlotId);
		packet.put_integer("JOIN", state);
		packet.put_integer("MLIM", memberLimit);
		packet.put_string("NAME", name);
		packet.put_integer("NTOP", ntop);
		packet.put_integer("OWNR", ownerId);
		packet.put_integer("PGID", playgroupId);
		packet.put_integer("PRES", pres);
		packet.put_string("UKEY", ukey);
		packet.put_integer("UPRS", uprs ? 1 : 0);
		packet.put_string("UUID", uuid);
		packet.put_integer("VOIP", VoipTopology::Disabled);
		packet.put_blob("XNNC", nullptr, 0);
		packet.put_blob("XSES", nullptr, 0);
	}

	// PlaygroupMemberInfo
	void PlaygroupMemberInfo::Write(TDF::Packet& packet) const {
		packet.push_map("ATTR", TDF::Type::Integer, TDF::Type::String);
		for (const auto& [key, value] : attributes) {
			packet.put_string(key, value);
		}
		packet.pop();

		packet.put_integer("JTIM", jtim);
		packet.put_integer("PERM", permissions);

		packet.push_union("PNET", NetworkAddressMember::Unset);
		// TODO: check if this is neccesary
		packet.pop();

		packet.put_integer("SID", slot);

		packet.push_struct("USER");
		user.Write(packet);
		packet.pop();
	}
	
	// PlayerConnectionStatus
	void PlayerConnectionStatus::Read(const rapidjson::Value& value) {
		if (value.IsObject()) {
			flags = value["FLGS"].GetUint();
			id = value["PID"].GetInt64();
			status = static_cast<ConnectionStatus>(value["STAT"].GetUint());
		}
	}

	void PlayerConnectionStatus::Write(TDF::Packet& packet) const {
		packet.put_integer("FLGS", flags);
		packet.put_integer("PID", id);
		packet.put_integer("STAT", status);
	}

	// ReplicatedGamePlayer
	void ReplicatedGamePlayer::Write(TDF::Packet& packet) const {
		packet.put_blob("BLOB", nullptr, 0);
		packet.put_integer("EXID", 0); // u64
		packet.put_integer("GID", gameId);
		packet.put_integer("LOC", localization);
		packet.put_string("NAME", name);

		packet.push_map("PATT", TDF::Type::String, TDF::Type::String);
		// player attributes
		packet.pop();

		packet.put_integer("PID", id);
#if 0
		packet.push_struct("PNET");
		hostNetwork.Write(packet);
		packet.pop();
#else
		packet.push_union("PNET", NetworkAddressMember::IpPairAddress);
		{
			packet.push_struct("VALU");
			hostNetwork.Write(packet);
			packet.pop();
		}
		packet.pop();
#endif
		packet.put_integer("SID", slot);
		packet.put_integer("SLOT", slotType);
		packet.put_integer("STAT", state);
		packet.put_integer("TIDX", tIndex);
		packet.put_integer("TIME", time);
		packet.put_object_id("UGID", ugid);
		packet.put_integer("UID", uid);
	}

	// ReplicatedGameData
	void ReplicatedGameData::Write(TDF::Packet& packet) const {
		packet.push_list("ADMN", TDF::Type::Integer);
		for (const auto value : administrators) {
			packet.put_integer("", value);
		}
		packet.pop();

		packet.push_map("ATTR", TDF::Type::String, TDF::Type::String);
		for (const auto& [key, value] : attributes) {
			packet.put_string(key, value);
		}
		packet.pop();

		packet.push_list("CAP", TDF::Type::Integer);
		for (const auto value : capacity) {
			packet.put_integer("", value);
		}
		packet.pop();

		packet.push_map("CRIT", TDF::Type::String, TDF::Type::String);
		for (const auto& [key, value] : criteria) {
			packet.put_string(key, value);
		}
		packet.pop();

		packet.put_integer("GID", id);
		packet.put_string("GNAM", name);
		packet.put_integer("GPVH", gpvh);
		packet.put_integer("GSET", settings);
		packet.put_integer("GSID", gsid);
		packet.put_integer("GSTA", state);
		packet.put_string("GTYP", type);

		packet.push_list("HNET", TDF::Type::Struct, true);
		{
#if 1
			packet.push_struct("");
			hostNetwork.Write(packet);
			packet.pop();
#else
			packet.push_union("", NetworkAddressMember::IpPairAddress);
			{
				packet.push_struct("VALU");
				hostNetwork.Write(packet);
				packet.pop();
			}
			packet.pop();
#endif
		}
		packet.pop();

		packet.put_integer("HSES", hses);
		packet.put_integer("IGNO", ignore ? 1 : 0);
		/*
		packet.push_map("MATR", TDF::Type::Integer, TDF::Type::Integer);
		std::map<std::string, std::string>();
		packet.pop();
		*/
		packet.put_integer("MCAP", maxPlayers);

		packet.push_struct("NQOS");
		networkQos.Write(packet);
		packet.pop();

		packet.put_integer("NRES", resetable ? 0 : 1);
		packet.put_integer("NTOP", networkTopology);
		packet.put_string("PGID", playgroupId);
		packet.put_blob("PGSR", nullptr, 0); // ?

		packet.push_struct("PHST");
		pHost.Write(packet);
		packet.pop();

		packet.put_integer("PRES", presence);
		packet.put_string("PSAS", psas);
		packet.put_integer("QCAP", queueCapacity);
		packet.put_integer("SEED", seed);
		packet.put_integer("TCAP", tcap);

		packet.push_struct("THST");
		tHost.Write(packet);
		packet.pop();

		packet.push_list("TIDS", TDF::Type::Integer);
		for (const auto value : tids) {
			packet.put_integer("", value);
		}
		packet.pop();

		packet.put_string("UUID", uuid);
		packet.put_integer("VOIP", VoipTopology::Disabled);
		packet.put_string("VSTR", version);
		packet.put_blob("XNNC", nullptr, 0);
		packet.put_blob("XSES", nullptr, 0);
	}
}
