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
	db_thread_(nullptr),
	endpoint_(nullptr)
{

}

Ctx::~Ctx()
{

}

EndPoint Ctx::identify()
{
	EndPoint point;
	point.type = this->yamlAs<uint32_t>({ "identify","type" }, 0);
	point.id = this->yamlAs<uint32_t>({ "identify","id" }, 0);
	point.auth = this->yamlAs<std::string>({ "identify","auth" }, "");
	return point;
}

std::shared_ptr<SelfRegistration> Ctx::getEndpoint()
{
	return endpoint_;
}

uint64_t Ctx::getNowMilliseconds()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	return milliseconds.count();
}

uint64_t Ctx::getNowSeconds()
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


void Ctx::init(const std::string& configFile)
{
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

		bool bDaemon = APie::CtxSingleton::get().yamlAs<bool>({"daemon"}, true);
		if (bDaemon)
		{
			this->daemonize();
		}

		s_log_name = APie::CtxSingleton::get().yamlAs<std::string>({ "log", "name" }, "apie");

		uint32_t pid = APie::Api::OsSysCallsSingleton::get().getCurProcessId();
		snprintf(timebuf, sizeof(timebuf), "%s-%d", m_launchTime.c_str(), pid);
		s_log_postfix = timebuf;

		PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "config:%s", configFile.c_str());

		adjustOpenFilesLimit();
		enableCoreFiles();

		uint32_t id = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","id" }, 0);
		APie::CtxSingleton::get().setServerId(id);

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

		bool enable = APie::CtxSingleton::get().yamlAs<bool>({ "mysql","enable" }, false);
		if (enable)
		{
			std::string host = APie::CtxSingleton::get().yamlAs<std::string>({ "mysql","host" }, "");
			std::string user = APie::CtxSingleton::get().yamlAs<std::string>({ "mysql","user" }, "");
			std::string passwd = APie::CtxSingleton::get().yamlAs<std::string>({ "mysql","passwd" }, "");
			std::string db = APie::CtxSingleton::get().yamlAs<std::string>({ "mysql","db" }, "");
			uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "mysql","port" }, 0);

			MySQLConnectOptions options;
			options.host = host;
			options.user = user;
			options.passwd = passwd;
			options.db = db;
			options.port = port;

			//db_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_DB, this->generatorTId());
			logic_thread_->initMysql(options);
		}


		for (const auto& item : this->node_["redis_clients"])
		{
			auto iType = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(item, { "client","type" }, 0);
			auto iId = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(item, { "client","id" }, 0);
			auto sHost = APie::CtxSingleton::get().nodeYamlAs<std::string>(item, { "client","host" }, "");
			auto iPort = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(item, { "client","port" }, 0);
			auto sPasswd = APie::CtxSingleton::get().nodeYamlAs<std::string>(item, { "client","passwd" }, "");

			auto key = std::make_tuple(iType, iId);

			auto ptrCb = [](std::shared_ptr<RedisClient> ptrClient) {
				std::stringstream ss;
				ss << "status:" << (uint32_t)ptrClient->getState();
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

	this->handleSigProcMask();
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

	if (db_thread_ != nullptr && db_thread_->state() == Event::DTState::DTS_Ready)
	{
		db_thread_->start();
		thread_id_[db_thread_->getTId()] = db_thread_;
	}
	

	Command command;
	command.type = Command::logic_start;
	command.args.logic_start.iThreadId = APie::CtxSingleton::get().getLogicThread()->getTId();
	APie::CtxSingleton::get().getLogicThread()->push(command);
}

void Ctx::destroy()
{
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

	if (db_thread_ != nullptr)
	{
		db_thread_.reset();
	}
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
	sigemptyset(&g_SigSet);
	sigaddset(&g_SigSet, SIGTERM);
	//sigaddset(&g_SigSet, SIGINT);
	sigaddset(&g_SigSet, SIGHUP);
	sigaddset(&g_SigSet, SIGQUIT);

	//sigprocmask(SIG_BLOCK, &g_SigSet, NULL);
	int rc = pthread_sigmask(SIG_BLOCK, &g_SigSet, NULL);
	if ( rc != 0)
	{
		PANIC_ABORT("pthread_sigmask");
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
	bool bDaemon = APie::CtxSingleton::get().yamlAs<bool>({ "daemon" }, true);
	if (bDaemon)
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
				quitFlag = true;
				pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "Aborting nicely");
				break;
			default:
				pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "re sigwait");
				break;
			}
		}
	}
	else
	{
		while (true)
		{
			std::cout << std::endl;

			std::cout << ">>>";
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

YAML::Node& Ctx::yamlNode()
{
	return this->node_;
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

std::shared_ptr<Event::DispatchedThreadImpl> Ctx::getDBThread()
{
	return db_thread_;
}

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
