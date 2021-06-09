#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"
#include "../../../pb_msg/business/login_msg.pb.h"
#include "../../../pb_msg/business/rpc_login.pb.h"

namespace APie {

	class GatewayRole;

	struct PendingLoginRole
	{
		uint64_t role_id;
		std::string session_key;
		uint32_t db_id;
		uint64_t expires_at;
	};

	class GatewayMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

		std::shared_ptr<GatewayRole> findGatewayRoleById(uint64_t iRoleId);
		std::shared_ptr<GatewayRole> findGatewayRoleBySerialNum(uint64_t iSerialNum);
		std::optional<uint64_t> findRoleIdBySerialNum(uint64_t iSerialNum);

		void addPendingRole(const PendingLoginRole &role);
		std::optional<PendingLoginRole> getPendingRole(uint64_t iRoleId);
		void removePendingRole(uint64_t iRoleId);

		bool addGatewayRole(std::shared_ptr<GatewayRole> ptrGatewayRole);
		bool removeGateWayRole(uint64_t iRoleId);

	public:
		// CMD
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);
		
		static void onMysqlInsertToDbORM(::pubsub::LOGIC_CMD& cmd);
		static void onMysqlDeleteFromDbORM(::pubsub::LOGIC_CMD& cmd);
		static void onMysqlUpdateToDbORM(::pubsub::LOGIC_CMD& cmd);
		static void onMysqlLoadFromDbORM(::pubsub::LOGIC_CMD& cmd);
		static void onMysqlQueryFromDbORM(::pubsub::LOGIC_CMD& cmd);


		// RPC
		static std::tuple<uint32_t, std::string> RPC_handleDeMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::rpc_msg::PRC_DeMultiplexer_Forward_Args& request);
		static std::tuple<uint32_t, std::string> RPC_handleLoginPending(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::rpc_login::L2G_LoginPendingRequest& request);

		

		// PubSub
		static void onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg);


		// CLIENT OPCODE
		static void handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg);

		static void handleRequestClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_CLIENT_LOGIN& request);
		static void handleRequestHandshakeInit(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_HANDSHAKE_INIT& request);
		static void handleRequestHandshakeEstablished(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_HANDSHAKE_ESTABLISHED& request);
		

	private:
		std::map<uint64_t, std::shared_ptr<GatewayRole>> m_serialNumMap; // key:serialNum, value:shared_ptr
		std::map<uint64_t, uint64_t> m_roleIdMapSerialNum;               // key:roleId, value:serialNum

		std::unordered_map<uint64_t, PendingLoginRole> m_pendingRole;
	};

	using GatewayMgrSingleton = ThreadSafeSingleton<GatewayMgr>;
}
