#pragma once

#include <string>
#include <stdint.h>
#include <queue>
#include <memory>
#include <queue>
#include <functional>
#include <map>

#include "i_poll_events.hpp"

#include "../event/timer.h"

#include <google/protobuf/message.h>

namespace APie
{
	class ClientProxy : public std::enable_shared_from_this<ClientProxy>
	{
	public:
		//iResult:0(连接成功)
		using HandleConnectCB = std::function<bool(APie::ClientProxy* ptrClient, uint32_t iResult)>;
		using HeartbeatCB = std::function<void(ClientProxy* ptrClient)>;

		enum CONNECT_STATE
		{
			CONNECT_CLOSE = 0,
			CONNECT_ESTABLISHED,
		};

		ClientProxy();
		~ClientProxy();
		bool checkTag();

		int connect(const std::string& ip, uint16_t port, ProtocolType type, uint32_t maskFlag, HandleConnectCB cb=nullptr);
		void resetConnect(const std::string& ip, uint16_t port, ProtocolType type);
		int reconnect();

		void addReconnectTimer(uint64_t interval);
		void disableReconnectTimer();

		void setHeartbeatCb(HeartbeatCB cb);
		void addHeartbeatTimer(uint64_t interval);
		void disableHeartbeatTimer();

		uint32_t getReconnectTimes();
		void onConnect(uint32_t iResult);
		void onPassiveClose(uint32_t iResult, const std::string& sInfo, uint32_t iActiveClose);
		void onActiveClose();

		void onRecvPackage(uint64_t iSerialNum, ::google::protobuf::Message* ptrMsg);

		int32_t sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg);

		uint64_t getSerialNum();
		std::string getCurSerialNumToStr();

		ProtocolType getCodecType();
		std::string getHosts();

		bool isConnectted();
		void setHadEstablished(uint32_t value);

		Event::TimerPtr& reconnectTimer();

		void setLocalIp(const std::string& ip);

	private:
		void close();
		int sendConnect();
		void sendClose();
		static uint64_t generatorId();

	public:
		static bool registerClientProxy(std::shared_ptr<ClientProxy> ptrClient);
		static void unregisterClientProxy(uint64_t iSerialNum);
		static std::shared_ptr<ClientProxy> findClientProxy(uint64_t iSerialNum);

		static void clearAllClientProxy();

		static std::shared_ptr<ClientProxy> createClientProxy();

	private:
		uint32_t m_tag;
		uint64_t m_curSerialNum;
		std::string m_localIp;

		std::string m_ip;
		uint16_t m_port;
		ProtocolType m_codecType;  //协议：1(PB√),2(HTTP)
		uint32_t m_maskFlag = 0;   //按位表示：0b1u(压缩√,客户端<->Gateway),0b10u(加密×)
		HandleConnectCB m_cb;

		uint32_t m_hadEstablished; //当前的连接状态：0：未连接，1：已连上
		uint32_t m_reconnectTimes;

		Event::TimerPtr m_reconnectTimer;

		Event::TimerPtr m_heartbeatTimer;
		HeartbeatCB m_heartbeatCb;

		uint32_t m_tId;            //附着IO线程ID

		static std::mutex m_sync;
		static std::map<uint64_t, std::shared_ptr<ClientProxy>> m_clientProxy;
	};

}
