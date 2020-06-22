#include <new>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <iostream>

#include "../network/Ctx.h"
#include "../network/address.h"
#include "../network/client_proxy.h"

#include "../common/exception_trap.h"

#include "../api/hook.h"

#ifdef WIN32
#define SLEEP_MS(ms) Sleep(ms)
#else
#define SLEEP_MS(ms) usleep((ms) * 1000)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "logger.h"




namespace APie {

PlatformImpl Ctx::s_platform;

class PortCb : public Network::ListenerCallbacks
{
	void onAccept(evutil_socket_t fd)
	{
		std::cout << fd << std::endl;

		auto ptrAddr = Network::addressFromFd(fd);
		if (ptrAddr != nullptr)
		{
			std::string addr = Network::makeFriendlyAddress(*ptrAddr);
		}

		PassiveConnect *itemObjPtr = new PassiveConnect;
		itemObjPtr->iFd = fd;
		itemObjPtr->iType = ProtocolType::PT_PB;

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

};

Ctx::Ctx() :
	logic_thread_(nullptr),
	log_thread_(nullptr)
{

}

Ctx::~Ctx()
{

}

void Ctx::init()
{
	APie::ExceptionTrap();

	APie::Event::Libevent::Global::initialize();

	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Init);

	auto ptrListen = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Listen, this->generatorTId());
	auto ptrCb = std::make_shared<PortCb>();
	ptrListen->push(ptrListen->dispatcher().createListener(ptrCb, 5007, 1024));

	thread_[Event::EThreadType::TT_Listen].push_back(ptrListen);

	thread_[Event::EThreadType::TT_IO].push_back(std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_IO, this->generatorTId()));
	//thread_[Event::EThreadType::TT_IO].push_back(std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_IO, this->generatorTId()));

	logic_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Logic, this->generatorTId());
	log_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Log, this->generatorTId());
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

	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Start);
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

		SLEEP_MS(1000);
	}
	//----------------------2:sleep----------------------------
	SLEEP_MS(1000);


	//----------------------3:delete----------------------------
	thread_id_.clear();

	thread_.erase(APie::Event::EThreadType::TT_Listen);
	thread_.erase(APie::Event::EThreadType::TT_IO);
	//thread_.erase(APie::Event::EThreadType::TT_Logic);
	//thread_.erase(APie::Event::EThreadType::TT_Log);
	logic_thread_.reset();
	log_thread_.reset();
}

uint32_t Ctx::generatorTId()
{
	++tid_;
	return tid_;
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
