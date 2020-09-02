#include "../event/dispatched_thread.h"

#include <chrono>
#include <memory>

#include "../common/time.h"
#include "../event/dispatcher.h"
#include "../network/logger.h"


namespace APie {
namespace Event {

void DispatchedThreadImpl::start(void) {
	state_ = DTState::DTS_Running;
	thread_ = std::thread([this]() -> void { threadRoutine(); });
}

void DispatchedThreadImpl::stop()
{
	this->sendStop();
}

void DispatchedThreadImpl::initMysql(MySQLConnectOptions& options)
{
	this->mysqlConnector_.init(options);
	bool bResult = this->mysqlConnector_.connect();
	if (!bResult)
	{
		std::stringstream ss;
		ss << "DbThread::init mysql_connector connect error, " << options.host << ":" << options.user << ":" << options.passwd << ":" << options.db << ":" << options.port;
		fatalExit(ss.str().c_str());
	}
}

DTState DispatchedThreadImpl::state()
{
	return state_;
}

uint32_t DispatchedThreadImpl::getTId()
{
	return tid_;
}

void DispatchedThreadImpl::exit() {
	if (thread_.joinable()) {
		//dispatcher_->exit();
		thread_.join();
		//listener_.clear();
	}
	state_ = DTState::DTS_Done;
}

void DispatchedThreadImpl::push(std::shared_ptr<Network::Listener> listener)
{
	listener_.push_back(listener);
}

void DispatchedThreadImpl::push(Command& cmd)
{
	dispatcher_->push(cmd);
}

void DispatchedThreadImpl::sendStop()
{
	Command command;
	command.type = Command::stop_thread;
	command.args.stop_thread.iThreadId = this->tid_;

	this->push(command);
}

void DispatchedThreadImpl::threadRoutine(void) 
{
	dispatcher_->start();
	dispatcher_->run();
	state_ = DTState::DTS_Exit;
	//dispatcher_.reset();
}

} // namespace Event
} // namespace Envoy
