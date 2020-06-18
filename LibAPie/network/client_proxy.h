#pragma once


#include <string>
#include <stdint.h>
#include <queue>
#include <memory>
#include <queue>
#include <functional>
#include <map>

#include "i_poll_events.hpp"
#include <google/protobuf/message.h>

namespace APie
{
	class ClientProxy : public std::enable_shared_from_this<ClientProxy>
	{
	public:
		enum CONNECT_STATE
		{
			CONNECT_CLOSE = 0,
			CONNECT_ESTABLISHED,
		};

		ClientProxy();
		~ClientProxy();

		bool checkTag();

		int connect(const std::string& ip, uint16_t port, ProtocolType type);
		void onConnect(uint32_t iResult);
		void onPassiveClose(uint32_t iResult, const std::string& sInfo, uint32_t iActiveClose);

		void onRecvPackage(uint64_t iSerialNum, ::google::protobuf::Message* ptrMsg);

		int32_t sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg);

		uint64_t getSerialNum();
		std::string getCurSerialNumToStr();

		ProtocolType getCodecType();
		std::string getHosts();


	private:
		void close();
		void sendClose();
		static uint64_t generatorId();

	public:
		static bool registerClient(std::shared_ptr<ClientProxy> ptrClient);
		static void unregisterClient(uint64_t iSerialNum);
		static std::shared_ptr<ClientProxy> findClient(uint64_t iSerialNum);

	private:
		uint32_t m_tag;
		uint64_t m_curSerialNum;

		std::string m_ip;
		uint16_t m_port;
		ProtocolType m_codecType;

		uint32_t m_hadEstablished; //当前的连接状态：0：未连接，1：已连上


		static std::mutex m_sync;
		static std::map<uint64_t, std::shared_ptr<ClientProxy>> m_clientProxy;
	};

}
