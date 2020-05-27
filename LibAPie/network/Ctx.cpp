#include <new>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <iostream>

#include "../network/Ctx.h"
#include "../network/address.h"

#ifdef WIN32
#define SLEEP_MS(ms) Sleep(ms)
#else
#define SLEEP_MS(ms) usleep((ms) * 1000)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif



namespace APie {

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
	logic_thread_(nullptr)
{

}

Ctx::~Ctx()
{

}

void Ctx::init()
{
	auto ptrListen = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Listen, this->generatorTId());
	auto ptrCb = std::make_shared<PortCb>();
	ptrListen->push(ptrListen->dispatcher().createListener(ptrCb, 5007, 1024));

	thread_[Event::EThreadType::TT_Listen].push_back(ptrListen);

	thread_[Event::EThreadType::TT_IO].push_back(std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_IO, this->generatorTId()));
	thread_[Event::EThreadType::TT_IO].push_back(std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_IO, this->generatorTId()));

	logic_thread_ = std::make_shared<Event::DispatchedThreadImpl>(Event::EThreadType::TT_Logic, this->generatorTId());
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
}

void Ctx::destroy()
{

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
