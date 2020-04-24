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

#include "google/protobuf/message.h"

#include "../../PBMsg/login_msg.pb.h"

using namespace Envoy;


int main(int argc, char **argv)
{
	Event::Libevent::Global::initialize();
	PlatformImpl platform;

	Envoy::CtxSingleton::get().init();
	Envoy::CtxSingleton::get().start();

	auto ptrCb = [](uint64_t serialNum, std::shared_ptr<::google::protobuf::Message> ptrMsg) {
		::login_msg::MSG_CLIENT_LOGINTOL *ptrReqeust = dynamic_cast<::login_msg::MSG_CLIENT_LOGINTOL *>(ptrMsg.get());
		if (ptrReqeust == nullptr)
		{
			return;
		}
		std::cout << "serialNum:" << serialNum << std::endl;
		ptrReqeust->PrintDebugString();
	};
	Api::PBHandlerSingleton::get().registerHandler(1100, ::login_msg::MSG_CLIENT_LOGINTOL(), ptrCb);
	std::cin.get();

    return 0;
}
