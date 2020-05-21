#include "network/windows_platform.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#include <windows.h>
#include <direct.h>

#include "yaml-cpp/yaml.h"

#include "event/real_time_system.h"
#include "event/dispatched_thread.h"
#include "event/libevent_scheduler.h"

#include "api/api_impl.h"
#include "api/pb_handler.h"

#include "network/platform_impl.h"
#include "network/Ctx.h"
#include "network/output_stream.h"

#include "google/protobuf/message.h"

#include "../../PBMsg/login_msg.pb.h"

using namespace Envoy;

class dispatcher {
public:
	template <typename F, typename Params>
	void bind(uint64_t id, F func, Params params)
	{
		auto ptrCb = [id,func, params](::google::protobuf::Message& stMsg) mutable {
			try
			{
				params.CopyFrom(stMsg);
			}
			catch (const std::exception& a)
			{
				//std::cout << "e:" << e.what() << std::endl;
			}
			func(params);
		};
		funcs_[id] = ptrCb;
	}

	using adaptor_type = std::function<void(::google::protobuf::Message& stMsg)>;
	std::map<uint64_t, adaptor_type> funcs_;
};

void fun_1(::login_msg::MSG_CLIENT_LOGINTOL params)
{
	std::cout << "params:"<< params.DebugString() << std::endl;
}

void fun_2(::login_msg::MSG_LOGINSERVER_VALIDATE params)
{
	std::cout << "params:" << params.DebugString() << std::endl;
}


#define DEFINE_TYPE_TRAIT(name, func)                            \
  template <typename T>                                          \
  class name {                                                   \
   private:                                                      \
    template <typename Class>                                    \
    static char Test(decltype(&Class::func)*);                   \
    template <typename>                                          \
    static int Test(...);                                        \
                                                                 \
   public:                                                       \
    static constexpr bool value = sizeof(Test<T>(nullptr)) == 1; \
  };                                                             \
                                                                 \
  template <typename T>                                          \
  constexpr bool name<T>::value;


DEFINE_TYPE_TRAIT(HasShutdown, Shutdown)

template <typename T>
typename std::enable_if<HasShutdown<T>::value>::type CallShutdown(T *instance) {
	instance->Shutdown();
}

//template <typename T>
//typename std::enable_if<!HasShutdown<T>::value>::type CallShutdown(
//	T *instance) {
//	(void)instance;
//}

class TestA1
{
public:

	void Shutdown() {
		std::cout << "aaaaa" << std::endl;
	}


};


template <typename MessageT,
	typename std::enable_if<
	std::is_base_of<google::protobuf::Message, MessageT>::value,
	int>::type = 0>
	std::string MessageType(const MessageT& message) {
	return message.GetDescriptor()->full_name();
}


int main(int argc, char **argv)
{
	decltype(&TestA1::Shutdown) varT;

	TestA1 testa;

	CallShutdown(&testa);

	Event::Libevent::Global::initialize();
	PlatformImpl platform;

	Envoy::CtxSingleton::get().init();
	Envoy::CtxSingleton::get().start();


	::login_msg::MSG_CLIENT_LOGINTOL reqeust;
	reqeust.set_user_id(100);


	std::string msgType = MessageType(reqeust);

	::login_msg::MSG_LOGINSERVER_VALIDATE reqeust1;
	reqeust1.set_port(200);

	dispatcher a;
	a.bind(1, fun_1, ::login_msg::MSG_CLIENT_LOGINTOL());
	a.bind(2, fun_2, ::login_msg::MSG_LOGINSERVER_VALIDATE());

	

	a.funcs_[1](reqeust);
	a.funcs_[2](reqeust);

	auto ptrCb = [](uint64_t serialNum, std::shared_ptr<::google::protobuf::Message> ptrMsg) {
		::login_msg::MSG_CLIENT_LOGINTOL *ptrReqeust = dynamic_cast<::login_msg::MSG_CLIENT_LOGINTOL *>(ptrMsg.get());
		if (ptrReqeust == nullptr)
		{
			return;
		}
		std::cout << "serialNum:" << serialNum << std::endl;
		ptrReqeust->PrintDebugString();


		::login_msg::MSG_CLIENT_LOGINTOL response;
		response.set_user_id(ptrReqeust->user_id());

		Network::OutputStream::sendMsg(serialNum, 1101, response);
	};
	Api::PBHandlerSingleton::get().registerHandler(1100, ::login_msg::MSG_CLIENT_LOGINTOL(), ptrCb);
	std::cin.get();

    return 0;
}
