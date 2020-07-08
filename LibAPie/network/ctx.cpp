#include <new>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

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

#define SLEEP_MS(ms) usleep((ms) * 1000)

sigset_t g_SigSet;
#endif

#include "logger.h"
#include "i_poll_events.hpp"



namespace APie {

PlatformImpl Ctx::s_platform;

class PortCb : public Network::ListenerCallbacks
{
public:
	PortCb(ProtocolType type) : m_type(type)
	{

	}

	void onAccept(evutil_socket_t fd)
	{
		//std::cout << fd << std::endl;

		std::string ip;
		auto ptrAddr = Network::addressFromFd(fd);
		if (ptrAddr != nullptr)
		{
			ip = Network::makeFriendlyAddress(*ptrAddr);
		}

		PassiveConnect *itemObjPtr = new PassiveConnect;
		itemObjPtr->iFd = fd;
		itemObjPtr->iType = m_type;
		itemObjPtr->sIp = ip;

		Command command;
		command.type = Command::passive_connect;
		command.args.passive_connect.ptrData = itemObjPtr;

		auto ptrThread = APie::CtxSingleton::get().chooseIOThread();
		if (ptrThread == nullptr)
		{
			return;
		}
		ptrThread->push(command);
	}

private:
	ProtocolType m_type;
};

Ctx::Ctx() :
	logic_thread_(nullptr),
	log_thread_(nullptr),
	metrics_thread_(nullptr)
{

}

Ctx::~Ctx()
{

}

void Ctx::init(const std::string& configFile)
{
	APie::ExceptionTrap();

	APie::Event::Libevent::Global::initialize();

	this->handleSigProcMask();

	try {
		this->node_ = YAML::LoadFile(configFile);

		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Init);

		for (const auto& item : this->node_["listeners"])
		{
			std::string ip = item["address"]["socket_address"]["address"].as<std::string>();
			uint16_t port = item["address"]["socket_address"]["port_value"].as<uint16_t>();
			uint16_t type = item["address"]["socket_address"]["type"].as<uint16_t>();

			Network::ListenerConfig config;
			config.ip = ip;
			config.port = port;
			config.type = static_cast<APie::ProtocolType>(type);

			auto ptrCb = std::make_shared<PortCb>(config.type);

			auto ptrListen = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Listen, this->generatorTId());
			ptrListen->push(ptrListen->dispatcher().createListener(ptrCb, config));
			thread_[Event::EThreadType::TT_Listen].push_back(ptrListen);

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
	}
	catch (YAML::BadFile& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|BadFile exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "fatalExit", ss.str().c_str());
		throw;
	}
	catch (YAML::InvalidNode& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|InvalidNode exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "fatalExit", ss.str().c_str());
		throw;
	}
	catch (std::exception& e) {
		std::stringstream ss;
		ss << "fileName:" << configFile << "|Unexpected exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "fatalExit", ss.str().c_str());
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
	

	Command command;
	command.type = Command::logic_start;
	command.args.logic_start.iThreadId = APie::CtxSingleton::get().getLogicThread()->getTId();
	APie::CtxSingleton::get().getLogicThread()->push(command);
}

void Ctx::destroy()
{
	APie::Event::DispatcherImpl::clearAllConnection();
	ClientProxy::clearClientProxy();

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

	sigprocmask(SIG_BLOCK, &g_SigSet, NULL);
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

			auto ptrCmd = new LogicCmd;
			ptrCmd->sCmd = mystring;

			Command command;
			command.type = Command::logic_cmd;
			command.args.logic_cmd.ptrData = ptrCmd;
			APie::CtxSingleton::get().getLogicThread()->push(command);
		}
	}
#else
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
				fatalExit("sigwait error exit");
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
