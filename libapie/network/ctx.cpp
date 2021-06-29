#include <new>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <tuple> 

#include "../network/ctx.h"
#include "../network/address.h"
#include "../network/client_proxy.h"

#include "../common/exception_trap.h"

#include "../api/hook.h"

#include "../event/nats_proxy.h"


#ifdef WIN32
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <signal.h>
#include <dirent.h>
#include <libgen.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SLEEP_MS(ms) usleep((ms) * 1000)

sigset_t g_SigSet;
#endif

#include "logger.h"
#include "i_poll_events.hpp"
#include "../common/string_utils.h"
#include "../api/os_sys_calls.h"

#include "../redis_driver/redis_client.h"
#include "../common/file.h"
#include "nats/nats.h"


namespace APie {

PlatformImpl Ctx::s_platform;
std::string Ctx::s_log_name;
std::string Ctx::s_log_postfix;

class PortCb : public Network::ListenerCallbacks
{
public:
	PortCb(ProtocolType type, uint32_t maskFlag) : 
		m_type(type),
		m_maskFlag(maskFlag)
	{

	}

	void onAccept(evutil_socket_t fd)
	{
		std::string ip;
		std::string peerIp;
		auto ptrAddr = Network::addressFromFd(fd);
		if (ptrAddr != nullptr)
		{
			ip = Network::makeFriendlyAddress(*ptrAddr);
		}

		auto ptrPeerAddr = Network::peerAddressFromFd(fd);
		if (ptrPeerAddr != nullptr)
		{
			peerIp = Network::makeFriendlyAddress(*ptrPeerAddr);
		}


		PassiveConnect *itemObjPtr = new PassiveConnect;
		itemObjPtr->iFd = fd;
		itemObjPtr->iType = m_type;
		itemObjPtr->sIp = ip;
		itemObjPtr->sPeerIp = peerIp;
		itemObjPtr->iMaskFlag = m_maskFlag;

		Command command;
		command.type = Command::passive_connect;
		command.args.passive_connect.ptrData = itemObjPtr;

		std::stringstream ss;
		ss << "accept connect|fd:" << fd << "|iType:" << static_cast<uint32_t>(m_type) << "|peerIp:" << peerIp << " -> " << "ip:" << ip;
		ASYNC_PIE_LOG("PortCb/onAccept", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());


		auto ptrThread = APie::CtxSingleton::get().chooseIOThread();
		if (ptrThread == nullptr)
		{
			delete itemObjPtr;

			return;
		}
		ptrThread->push(command);
	}

private:
	ProtocolType m_type;
	uint32_t m_maskFlag;
};

Ctx::Ctx() :
	logic_thread_(nullptr),
	log_thread_(nullptr),
	metrics_thread_(nullptr),
	//db_thread_(nullptr),
	nats_thread_(nullptr),
	endpoint_(nullptr)
{
	this->m_ptrConfig = std::make_shared<APieConfig>();
}

Ctx::~Ctx()
{

}

EndPoint Ctx::identify()
{
	EndPoint point;
	point.type = this->getServerType();
	point.id = this->getServerId();
	point.auth = this->getConfigs()->identify.auth;
	return point;
}

std::shared_ptr<SelfRegistration> Ctx::getEndpoint()
{
	return endpoint_;
}

uint64_t Ctx::getCurMilliseconds()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	return milliseconds.count();
}

uint64_t Ctx::getCurSeconds()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	return seconds.count();
}

uint32_t Ctx::generateHash(EndPoint point)
{
	//Time33
	//uint32_t hashValue = 5381;

	uint32_t hashValue = 0;
	std::vector<uint32_t> keys;
	keys.push_back(point.type);
	keys.push_back(point.id);

	for (const auto& items : keys)
	{
		hashValue = ((hashValue << 5) + hashValue) + items;
	}

	return hashValue;
}

std::shared_ptr<APieConfig> Ctx::loadConfigs()
{
	auto tmpPtrConfig = std::make_shared<APieConfig>();

	if (node_["identify"])
	{
		tmpPtrConfig->identify.realm = node_["identify"]["realm"].as<uint16_t>(0);
		tmpPtrConfig->identify.type = node_["identify"]["type"].as<uint32_t>();
		tmpPtrConfig->identify.id = node_["identify"]["id"].as<uint32_t>();
		tmpPtrConfig->identify.auth = node_["identify"]["auth"].as<std::string>("");
		tmpPtrConfig->identify.ip = node_["identify"]["ip"].as<std::string>("");
		tmpPtrConfig->identify.port = node_["identify"]["port"].as<uint16_t>(0);
		tmpPtrConfig->identify.out_ip = node_["identify"]["out_ip"].as<std::string>("");
		tmpPtrConfig->identify.out_port = node_["identify"]["out_port"].as<uint16_t>(0);
		tmpPtrConfig->identify.codec_type = node_["identify"]["codec_type"].as<uint16_t>(1);
	}
	else
	{
		return nullptr;
	}	

	tmpPtrConfig->io_threads = node_["io_threads"].as<uint16_t>(2);
	tmpPtrConfig->daemon = node_["daemon"].as<bool>(true);
	tmpPtrConfig->service_timeout = node_["service_timeout"].as<uint32_t>(600);
	tmpPtrConfig->service_learning_duration = node_["service_learning_duration"].as<uint32_t>(60);

	if (node_["certificate"])
	{
		tmpPtrConfig->certificate.public_key = node_["certificate"]["public_key"].as<std::string>("");
		tmpPtrConfig->certificate.private_key = node_["certificate"]["private_key"].as<std::string>("");
	}

	for (const auto& item : this->node_["listeners"])
	{
		std::string ip = item["address"]["socket_address"]["address"].as<std::string>();
		uint16_t port = item["address"]["socket_address"]["port_value"].as<uint16_t>();
		uint16_t type = item["address"]["socket_address"]["type"].as<uint16_t>(1);
		uint32_t maskFlag = item["address"]["socket_address"]["mask_flag"].as<uint32_t>(0);

		APieConfig_ListenersElems elems;
		elems.socket_address.address = ip;
		elems.socket_address.port_value = port;
		elems.socket_address.type = type;
		elems.socket_address.mask_flag = maskFlag;

		tmpPtrConfig->listeners.push_back(elems);
	}

	if (this->node_["clients"])
	{
		std::string ip = node_["clients"]["socket_address"]["address"].as<std::string>();
		uint16_t port = node_["clients"]["socket_address"]["port_value"].as<uint16_t>();
		uint16_t type = node_["clients"]["socket_address"]["type"].as<uint16_t>(1);
		uint32_t maskFlag = node_["clients"]["socket_address"]["mask_flag"].as<uint32_t>(0);

		APieConfig_ClientsElems elems;
		elems.socket_address.address = ip;
		elems.socket_address.port_value = port;
		elems.socket_address.type = type;
		elems.socket_address.mask_flag = maskFlag;

		tmpPtrConfig->clients = elems;
	}

	if (node_["service_registry"])
	{
		tmpPtrConfig->service_registry.address = node_["service_registry"]["address"].as<std::string>("127.0.0.1");
		tmpPtrConfig->service_registry.port_value = node_["service_registry"]["port_value"].as<uint16_t>(5007);
		tmpPtrConfig->service_registry.auth = node_["service_registry"]["auth"].as<std::string>("");
		tmpPtrConfig->service_registry.type = node_["service_registry"]["type"].as<uint16_t>(1);
	}

	if (node_["log"])
	{
		tmpPtrConfig->log.merge = node_["log"]["merge"].as<bool>(true);
		tmpPtrConfig->log.level = node_["log"]["level"].as<uint16_t>(2);
		tmpPtrConfig->log.show_pos = node_["log"]["show_pos"].as<bool>(false);
		tmpPtrConfig->log.split_size = node_["log"]["split_size"].as<uint16_t>(128);
		tmpPtrConfig->log.backup = node_["log"]["backup"].as<std::string>("/usr/local/apie/logs/backup");
		tmpPtrConfig->log.name = node_["log"]["name"].as<std::string>("apie");
		tmpPtrConfig->log.show_console = node_["log"]["show_console"].as<bool>(false);
	}

	if (node_["metrics"])
	{
		tmpPtrConfig->metrics.enable = node_["metrics"]["enable"].as<bool>(false);
		tmpPtrConfig->metrics.ip = node_["metrics"]["ip"].as<std::string>("127.0.0.1");
		tmpPtrConfig->metrics.udp_port = node_["metrics"]["udp_port"].as<uint16_t>(8089);
	}

	if (node_["mysql"])
	{
		tmpPtrConfig->mysql.enable = node_["mysql"]["enable"].as<bool>(false);
		tmpPtrConfig->mysql.host = node_["mysql"]["host"].as<std::string>("127.0.0.1");
		tmpPtrConfig->mysql.port = node_["mysql"]["port"].as<uint16_t>(3306);
		tmpPtrConfig->mysql.user = node_["mysql"]["user"].as<std::string>("root");
		tmpPtrConfig->mysql.passwd = node_["mysql"]["passwd"].as<std::string>("root");
		tmpPtrConfig->mysql.db = node_["mysql"]["db"].as<std::string>("apie");
	}

	for (const auto& item : this->node_["redis_clients"])
	{
		uint32_t type = item["client"]["type"].as<uint32_t>();
		uint32_t id = item["client"]["id"].as<uint32_t>();
		std::string host = item["client"]["host"].as<std::string>();
		uint16_t port = item["client"]["port"].as<uint16_t>();
		std::string passwd = item["client"]["passwd"].as<std::string>();


		APieConfig_RedisClient elems;
		elems.type = type;
		elems.id = id;
		elems.host = host;
		elems.port = port;
		elems.passwd = passwd;

		tmpPtrConfig->redis_clients.push_back(elems);
	}

	if (node_["nats"])
	{
		tmpPtrConfig->nats.enable = node_["nats"]["enable"].as<bool>(false);

		for (const auto& item : this->node_["nats"]["connections"])
		{
			uint32_t type = item["subscription"]["type"].as<uint32_t>();
			std::string nats_server = item["subscription"]["nats_server"].as<std::string>();
			std::string channel_domains = item["subscription"]["channel_domains"].as<std::string>();


			APieConfig_NatsSubscription elems;
			elems.type = type;
			elems.nats_server = nats_server;
			elems.channel_domains = channel_domains;

			tmpPtrConfig->nats.connections.push_back(elems);
		}
	}

	if (node_["etcd"])
	{
		tmpPtrConfig->etcd.enable = node_["etcd"]["enable"].as<bool>(false);
		tmpPtrConfig->etcd.urls = node_["etcd"]["urls"].as<std::string>("");
		tmpPtrConfig->etcd.prefix = node_["etcd"]["prefix"].as<std::string>("");
	}

	if (node_["limited"])
	{
		tmpPtrConfig->limited.requests_per_unit = node_["limited"]["requests_per_unit"].as<uint32_t>(0);
		tmpPtrConfig->limited.uint = node_["limited"]["uint"].as<uint32_t>(60);
	}

	return tmpPtrConfig;
}

void Ctx::init(const std::string& configFile)
{
	this->m_bLogEnable = true;

	this->m_configFile = configFile;
	int64_t mtime = APie::Common::FileDataModificationTime(this->m_configFile);
	if (mtime == -1)
	{
		PANIC_ABORT("configFile:%s not exist", configFile.c_str());
	}
	this->setConfigFileMTime(mtime);

	time_t now = time(NULL);

	char timebuf[128] = { '\0' };
	strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S", localtime(&now));
	m_launchTime = timebuf;

	memset(timebuf, 0, sizeof(timebuf));

	APie::ExceptionTrap();

	APie::Event::Libevent::Global::initialize();

	endpoint_ = SelfRegistration::createSelfRegistration();

	try {
		this->node_ = YAML::LoadFile(configFile);
		auto ptrConfig = this->loadConfigs();
		if (ptrConfig == nullptr)
		{
			PANIC_ABORT("configFile:%s load error", configFile.c_str());
		}
		this->m_ptrConfig = ptrConfig;


		if (this->getConfigs()->daemon)
		{
			this->daemonize();
		}

		s_log_name = this->getConfigs()->log.name;

		uint32_t pid = APie::Api::OsSysCallsSingleton::get().getCurProcessId();
		snprintf(timebuf, sizeof(timebuf), "%s-%d", m_launchTime.c_str(), pid);
		s_log_postfix = timebuf;

		PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "config:%s", configFile.c_str());

		adjustOpenFilesLimit();
		enableCoreFiles();

		handleSigProcMask();

		uint32_t id = this->getConfigs()->identify.id; 
		uint32_t type = this->getConfigs()->identify.type;
		APie::CtxSingleton::get().setServerId(id);
		APie::CtxSingleton::get().setServerType(type);

		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Init);

		std::shared_ptr<Event::DispatchedThreadImpl> ptrListen = nullptr;

		for (const auto& item : this->node_["listeners"])
		{
			std::string ip = item["address"]["socket_address"]["address"].as<std::string>();
			uint16_t port = item["address"]["socket_address"]["port_value"].as<uint16_t>();
			uint16_t type = item["address"]["socket_address"]["type"].as<uint16_t>();
			uint32_t maskFlag = item["address"]["socket_address"]["mask_flag"].as<uint32_t>();

			Network::ListenerConfig config;
			config.ip = ip;
			config.port = port;
			config.type = static_cast<APie::ProtocolType>(type);

			if (config.type <= ProtocolType::PT_None || config.type >= ProtocolType::PT_MAX)
			{
				std::stringstream ss;
				ss << "invalid listener type:" << type;
				PANIC_ABORT(ss.str().c_str());
			}

			
			if (nullptr == ptrListen)
			{
				ptrListen = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Listen, this->generatorTId());
				thread_[Event::EThreadType::TT_Listen].push_back(ptrListen);
			}

			auto ptrCb = std::make_shared<PortCb>(config.type, maskFlag);
			ptrListen->push(ptrListen->dispatcher().createListener(ptrCb, config));

			PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "listeners|ip:%s|port:%d|type:%d", ip.c_str(), port, type);
		}

		uint16_t ioThreads = this->node_["io_threads"].as<uint16_t>();
		if (ioThreads <= 0 || ioThreads > 64)
		{
			ioThreads = 2;
		}
		for (uint32_t index = 0; index < ioThreads; index++)
		{
			thread_[Event::EThreadType::TT_IO].push_back(std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_IO, this->generatorTId()));
		}
		PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "ioThreads: %d", ioThreads);

		logic_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Logic, this->generatorTId());
		log_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Log, this->generatorTId());
		metrics_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Metrics, this->generatorTId());
		nats_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Nats, this->generatorTId());

		if (this->getConfigs()->mysql.enable)
		{
			std::string host = this->getConfigs()->mysql.host;
			std::string user = this->getConfigs()->mysql.user;
			std::string passwd = this->getConfigs()->mysql.passwd;
			std::string db = this->getConfigs()->mysql.db;
			uint16_t port = this->getConfigs()->mysql.port;

			MySQLConnectOptions options;
			options.host = host;
			options.user = user;
			options.passwd = passwd;
			options.db = db;
			options.port = port;

			//db_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_DB, this->generatorTId());
			logic_thread_->initMysql(options);
			PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "mysql:%s|%s|%d", host.c_str(), db.c_str(), port);
		}

		bool bResult = APie::Event::NatsSingleton::get().init();
		if (!bResult)
		{
			std::stringstream ss;
			ss << "nats init error";
			PANIC_ABORT(ss.str().c_str());
		}

		for (const auto& item : this->getConfigs()->redis_clients)
		{
			auto iType = item.type;
			auto iId = item.id;
			auto sHost = item.host;
			auto iPort = item.port;
			auto sPasswd = item.passwd;

			auto key = std::make_tuple(iType, iId);

			auto ptrCb = [](std::shared_ptr<RedisClient> ptrClient) {
				std::stringstream ss;
				ss << key_to_string(*ptrClient);
				ASYNC_PIE_LOG("RedisClient", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
			};
			auto sharedPtr = RedisClientFactorySingleton::get().createClient(key, sHost, iPort, sPasswd, ptrCb);
			bool bResult = RedisClientFactorySingleton::get().registerClient(sharedPtr);
			if (!bResult)
			{
				std::stringstream ss;
				ss << "redis|registerClient error|key:" << (uint32_t)std::get<0>(key) << "-" << std::get<1>(key);
				PANIC_ABORT(ss.str().c_str());
			}
		}
	}
	catch (YAML::BadFile& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|BadFile exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
	catch (YAML::InvalidNode& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|InvalidNode exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
	catch (YAML::BadConversion& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|BadConversion exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
	catch (std::exception& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|Unexpected exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
}

void Ctx::start()
{
	for (auto& item : thread_)
	{
		for (auto& elem : item.second)
		{
			if (elem->state() == Event::DTState::DTS_Ready)
			{
				elem->start();
				thread_id_[elem->getTId()] = elem;
			}
		}
	}

	if (logic_thread_->state() == Event::DTState::DTS_Ready)
	{
		logic_thread_->start();
		thread_id_[logic_thread_->getTId()] = logic_thread_;
	}

	if (log_thread_->state() == Event::DTState::DTS_Ready)
	{
		log_thread_->start();
		thread_id_[log_thread_->getTId()] = log_thread_;
	}

	if (metrics_thread_->state() == Event::DTState::DTS_Ready)
	{
		metrics_thread_->start();
		thread_id_[metrics_thread_->getTId()] = metrics_thread_;
	}

	if (nats_thread_->state() == Event::DTState::DTS_Ready)
	{
		nats_thread_->start();
		thread_id_[nats_thread_->getTId()] = nats_thread_;
	}

	//if (db_thread_ != nullptr && db_thread_->state() == Event::DTState::DTS_Ready)
	//{
	//	db_thread_->start();
	//	thread_id_[db_thread_->getTId()] = db_thread_;
	//}
	

	Command command;
	command.type = Command::logic_start;
	command.args.logic_start.iThreadId = APie::CtxSingleton::get().getLogicThread()->getTId();
	APie::CtxSingleton::get().getLogicThread()->push(command);
}

void Ctx::destroy()
{
	this->m_bLogEnable = false;

	APie::Event::DispatcherImpl::clearAllConnection();
	ClientProxy::clearAllClientProxy();

	//----------------------1:stop----------------------------
	for (auto& items : thread_id_)
	{
		items.second->stop();
	}

	bool bAllStop = false;
	while (!bAllStop)
	{
		bAllStop = true;
		for (auto& items : thread_id_)
		{
			if (items.second->state() != APie::Event::DTState::DTS_Exit)
			{
				bAllStop = false;
				break;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//SLEEP_MS(1000);
	}
	//----------------------2:sleep----------------------------
	//SLEEP_MS(1000);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));


	//----------------------3:delete----------------------------
	thread_id_.clear();

	thread_.erase(APie::Event::EThreadType::TT_Listen);
	thread_.erase(APie::Event::EThreadType::TT_IO);
	//thread_.erase(APie::Event::EThreadType::TT_Logic);
	//thread_.erase(APie::Event::EThreadType::TT_Log);
	logic_thread_.reset();
	log_thread_.reset();
	metrics_thread_.reset();
	nats_thread_.reset();

	//if (db_thread_ != nullptr)
	//{
	//	db_thread_.reset();
	//}

	logFileClose();
}

bool Ctx::logEnable()
{
	return this->m_bLogEnable;
}

void Ctx::daemonize()
{
#ifdef WIN32
#else
	int fd;

	umask(0);

	if (fork() != 0) exit(0); /* parent exits */
	setsid(); /* create a new session */

	/* Every output goes to /dev/null. If Redis is daemonized but
	 * the 'logfile' is set to 'stdout' in the configuration file
	 * it will not log at all. */
	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO) close(fd);
	}
#endif
}

bool Ctx::adjustOpenFilesLimit()
{
#ifdef WIN32
	return false;
#else
	int maxlimit = 1024 * 10;

	bool ret = false;
	struct rlimit limit;
	memset(&limit, 0, sizeof(limit));
	int e = getrlimit(RLIMIT_NOFILE, &limit);
	if (e < 0)
		return ret;
	struct rlimit newlimit;
	memset(&newlimit, 0, sizeof(newlimit));
	newlimit.rlim_cur = maxlimit;
	newlimit.rlim_max = maxlimit;
	if ((e = setrlimit(RLIMIT_NOFILE, &newlimit)) < 0)
	{
		setrlimit(RLIMIT_NOFILE, &limit);
	}
	else
	{
		ret = true;
	}
	return ret;
#endif
}

void Ctx::enableCoreFiles()
{
#ifdef WIN32
#else
	struct rlimit rlim, rlim_new;
	if (getrlimit(RLIMIT_CORE, &rlim) == 0)
	{
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
		{
			/* failed. try raising just to the old max */
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
			{
				printf("set core limit error\n");
			}
			else
			{
				printf("raising set core limit ok\n");
			}

}
		else
		{
			printf("original set core limit ok\n");
		}
	}
#endif
}

void Ctx::handleSigProcMask()
{
#ifdef WIN32
#else
	if (m_bDaemon) 
	{
		sigemptyset(&g_SigSet);
		sigaddset(&g_SigSet, SIGTERM);
		//sigaddset(&g_SigSet, SIGINT);
		sigaddset(&g_SigSet, SIGHUP);
		sigaddset(&g_SigSet, SIGQUIT);
		sigaddset(&g_SigSet, SIGUSR1);

		//sigprocmask(SIG_BLOCK, &g_SigSet, NULL);
		int rc = pthread_sigmask(SIG_BLOCK, &g_SigSet, NULL);
		if (rc != 0)
		{
			PANIC_ABORT("pthread_sigmask");
		}
	}
#endif
}

void Ctx::waitForShutdown()
{
#ifdef WIN32
	while (true)
	{
		std::cout << std::endl;

		std::cout << ">>>";
		char mystring[2048] = {'\0'};
		char answer[2048] = "exit";
		char* prtGet = fgets(mystring, 2048, stdin);
		if (prtGet != NULL)
		{
			//std::cout << "Input Recv:" << mystring << std::endl;
			int iResult = strncmp(mystring, answer, 4);
			if (iResult == 0)
			{
				PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Aborting nicely");
				break;
			}

			std::string cmd = APie::TrimString(mystring, APie::kWhitespaceASCII);
			if (cmd.empty())
			{
				continue;
			}

			auto ptrCmd = new LogicCmd;
			ptrCmd->sCmd = cmd;

			Command command;
			command.type = Command::logic_cmd;
			command.args.logic_cmd.ptrData = ptrCmd;
			APie::CtxSingleton::get().getLogicThread()->push(command);
		}
	}
#else
	if (m_bDaemon)
	{
		int actualSignal = 0;
		int errCount = 0;
		bool quitFlag = false;

		pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "handleSigWait");

		while (!quitFlag)
		{
			int status = sigwait(&g_SigSet, &actualSignal);
			if (status != 0)
			{
				pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Got error %d from sigwait", status);
				if (errCount++ > 5)
				{
					PANIC_ABORT("sigwait error exit");
				}
				continue;
			}

			errCount = 0;
			pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Main thread: Got signal %d|%s",
				actualSignal, strsignal(actualSignal));

			switch (actualSignal) {
			case SIGQUIT:
			case SIGTERM:
			case SIGHUP:
			{
				quitFlag = true;
				pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Aborting nicely");
				break;
			}
			case SIGUSR1:
			{
				auto ptrCmd = new LogicCmd;
				ptrCmd->sCmd = "reload";

				Command command;
				command.type = Command::logic_cmd;
				command.args.logic_cmd.ptrData = ptrCmd;
				APie::CtxSingleton::get().getLogicThread()->push(command);
				break;
			}
			default:
				pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "re sigwait");
				break;
			}
		}
	}
	else
	{
		uint64_t iIndex = 0;
		while (true)
		{
			if (iIndex > 100000000)
			{
				iIndex = 0;
			}

			//docker -d 0 -> /dev/null
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			iIndex++;
			std::cout << std::endl;
			std::cout << iIndex << ">>>";
			char mystring[2048] = { '\0' };
			char answer[2048] = "exit";
			char* prtGet = fgets(mystring, 2048, stdin);
			if (prtGet != NULL)
			{
				//std::cout << "Input Recv:" << mystring << std::endl;
				int iResult = strncmp(mystring, answer, 4);
				if (iResult == 0)
				{
					PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Aborting nicely");
					break;
				}

				std::string cmd = APie::TrimString(mystring, APie::kWhitespaceASCII);
				if (cmd.empty())
				{
					continue;
				}

				auto ptrCmd = new LogicCmd;
				ptrCmd->sCmd = cmd;

				Command command;
				command.type = Command::logic_cmd;
				command.args.logic_cmd.ptrData = ptrCmd;
				APie::CtxSingleton::get().getLogicThread()->push(command);
			}
		}
	}
#endif

	Command command;
	command.type = Command::logic_exit;
	command.args.logic_exit.iThreadId = APie::CtxSingleton::get().getLogicThread()->getTId();
	APie::CtxSingleton::get().getLogicThread()->push(command);

	while (!APie::CtxSingleton::get().getLogicThread()->dispatcher().terminating())
	{ 
		std::this_thread::yield(); 
	}
	this->destroy();
}

uint32_t Ctx::generatorTId()
{
	++tid_;
	return tid_;
}

//YAML::Node& Ctx::yamlNode()
//{
//	std::lock_guard<std::mutex> guard(node_sync_);
//	return this->node_;
//}

void Ctx::resetYamlNode(YAML::Node node)
{
	std::lock_guard<std::mutex> guard(node_sync_);
	this->node_ = node;

	auto ptrConfig = this->loadConfigs();
	if (ptrConfig == nullptr)
	{
		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_NOTICE, "reload|loadConfigs error");
		return;
	}

	this->m_ptrConfig = ptrConfig;
}

bool Ctx::yamlFieldsExists(std::vector<std::string> index)
{
	std::lock_guard<std::mutex> guard(node_sync_);

	std::vector<YAML::Node> nodeList;
	nodeList.push_back(node_);

	uint32_t iIndex = 0;

	for (const auto &items : index)
	{
		if (nodeList[iIndex][items])
		{
			nodeList.push_back(nodeList[iIndex][items]);
			iIndex++;
		}
		else
		{
			return false;
		}
	}

	return true;
}

std::string Ctx::launchTime()
{
	return m_launchTime;
}

uint32_t Ctx::getServerId()
{
	return m_server_id;
}

void Ctx::setServerId(uint32_t id)
{
	m_server_id = id;
}

uint32_t Ctx::getServerType()
{
	return m_server_type;
}

void Ctx::setServerType(uint32_t type)
{
	m_server_type = type;
}

bool Ctx::checkIsValidServerType(std::set<uint32_t> validSet)
{
	if (validSet.count(m_server_type) == 0)
	{
		return false;
	}

	return true;
}

bool Ctx::isDaemon()
{
	return m_bDaemon;
}

std::string Ctx::getConfigFile()
{
	return m_configFile;
}

int64_t Ctx::getConfigFileMTime()
{
	return m_configFileMTime;
}

void Ctx::setConfigFileMTime(int64_t mtime)
{
	m_configFileMTime = mtime;
}

std::shared_ptr<APieConfig> Ctx::getConfigs()
{
	return m_ptrConfig;
}

std::string Ctx::logName()
{
	return s_log_name;
}

std::string Ctx::logPostfix()
{
	return s_log_postfix;
}

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::chooseIOThread()
{
	static size_t iIndex = 0;
	iIndex++;

	size_t iSize = thread_[Event::EThreadType::TT_IO].size();
	if (iSize == 0)
	{
		return nullptr;
	}

	size_t iCur = iIndex % iSize;
	return thread_[Event::EThreadType::TT_IO][iCur];
}


std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getLogicThread()
{
	return logic_thread_;
}

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getLogThread()
{
	return log_thread_;
}

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getMetricsThread()
{
	return metrics_thread_;
}

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getNatsThread()
{
	return nats_thread_;
}

//std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getDBThread()
//{
//	return db_thread_;
//}

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getThreadById(uint32_t id)
{
	auto findIte = thread_id_.find(id);
	if (findIte == thread_id_.end())
	{
		return nullptr;
	}

	return findIte->second;
}

}
