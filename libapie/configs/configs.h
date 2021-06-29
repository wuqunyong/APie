#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>



namespace APie {

	struct APieConfig_Identify
	{
		uint32_t realm = 0;
		uint32_t type = 0;
		uint32_t id = 0;
		std::string auth;
		std::string ip;
		uint16_t port = 0;
		std::string out_ip;
		uint16_t out_port = 0;
		uint16_t codec_type = 1;
	};

	struct APieConfig_Certificate
	{
		std::string public_key;
		std::string private_key;
	};

	struct APieConfig_ListenersSocketAddress
	{
		std::string address;
		uint16_t port_value = 0;
		uint16_t type = 1;
		uint16_t mask_flag = 0;
	};

	struct APieConfig_ListenersElems
	{
		APieConfig_ListenersSocketAddress socket_address;
	};

	struct APieConfig_ClientsSocketAddress
	{
		std::string address;
		uint16_t port_value = 0;
		uint16_t type = 1;
		uint16_t mask_flag = 0;
	};

	struct APieConfig_ClientsElems
	{
		APieConfig_ClientsSocketAddress socket_address;
	};

	struct APieConfig_ServiceRegistry
	{
		std::string address;
		uint16_t port_value = 0;
		std::string auth;
		uint16_t type = 0;
	};

	struct APieConfig_Log
	{
		bool merge = true;
		uint16_t level = 0;
		bool show_pos = false;
		uint16_t split_size = 128;
		std::string backup;
		std::string name;
		bool show_console = false;
	};

	struct APieConfig_Metrics
	{
		bool enable = false;
		std::string ip;
		uint16_t udp_port = 8089;
	};

	struct APieConfig_Mysql
	{
		bool enable = false;
		std::string host;
		uint16_t port = 3306;
		std::string user;
		std::string passwd;
		std::string db;
	};

	struct APieConfig_RedisClient
	{
		uint32_t type = 0;
		uint32_t id = 0;
		std::string host;
		uint16_t port = 6379;
		std::string passwd;
	};

	struct APieConfig_NatsSubscription
	{
		uint32_t type = 0;
		std::string nats_server;
		std::string channel_domains;
	};

	struct APieConfig_Nats
	{
		bool enable = false;
		std::vector<APieConfig_NatsSubscription> connections;
	};

	struct APieConfig_Etcd
	{
		bool enable = false;
		std::string urls;
		std::string prefix;
	};

	struct APieConfig_Limited
	{
		uint32_t requests_per_unit = 0;
		uint32_t uint = 60;
	};


	struct APieConfig
	{
		APieConfig_Identify identify;
		uint16_t io_threads = 2;
		bool daemon = true;
		uint32_t service_timeout = 600;
		uint32_t service_learning_duration = 60;
		APieConfig_Certificate certificate;
		std::vector<APieConfig_ListenersElems> listeners;
		APieConfig_ClientsElems clients;
		APieConfig_ServiceRegistry service_registry;
		APieConfig_Log log;
		APieConfig_Metrics metrics;
		APieConfig_Mysql mysql;
		std::vector<APieConfig_RedisClient> redis_clients;
		APieConfig_Nats nats;
		APieConfig_Etcd etcd;
		APieConfig_Limited limited;
	};

} 
