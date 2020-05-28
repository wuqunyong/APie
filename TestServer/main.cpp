#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <locale.h>

#include "apie.h"

#include "../PBMsg/login_msg.pb.h"

using namespace APie;

class dispatcher {
public:
	template <typename F, typename Params, typename std::enable_if<
		std::is_base_of<google::protobuf::Message, Params>::value,
		int>::type = 0 >
	void bind(uint64_t id, F func, Params params)
	{
		using OriginType = Params;
		std::string sType = OriginType::descriptor()->full_name();

		auto ptrCb = [func, params](::google::protobuf::Message& stMsg) mutable {
			
			//try
			//{
			//	params.CopyFrom(stMsg);
			//}
			//catch (const std::exception& a)
			//{
			//	//std::cout << "e:" << e.what() << std::endl;
			//}
			OriginType &data = dynamic_cast<OriginType&>(stMsg);
			func(data);
		};


		funcs_[id] = ptrCb;
	}

	using adaptor_type = std::function<void(::google::protobuf::Message& stMsg)>;
	std::map<uint64_t, adaptor_type> funcs_;
};

void fun_1(::login_msg::MSG_CLIENT_LOGINTOL& params)
{
	std::cout << "params:"<< params.DebugString() << std::endl;
}

void fun_2(::login_msg::MSG_LOGINSERVER_VALIDATE& params)
{
	std::cout << "params:" << params.DebugString() << std::endl;
}

struct MyStruct11
{
	void CopyFrom(::google::protobuf::Message&)
	{

	}
};

void fun_3(MyStruct11 a)
{
	std::cout << "params:" << std::endl;
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

inline uint64_t StringToUnixSeconds( const std::string& time_str) 
{
	const std::string& format_str = "%d-%d-%d %d:%d:%d";

	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	int iResult = sscanf(time_str.c_str(), format_str.c_str(), 
		&tmp_time->tm_year, &tmp_time->tm_mon, &tmp_time->tm_mday,
		&tmp_time->tm_hour, &tmp_time->tm_min, &tmp_time->tm_sec);
	if (iResult != 6)
	{
		return 0;
	}

	tmp_time->tm_year -= 1900;
	tmp_time->tm_mon--;
	time_t time = mktime(tmp_time);
	free(tmp_time);
	return (uint64_t)time;
}

inline std::string UnixSecondsToString(
	uint64_t unix_seconds,
	const std::string& format_str = "%Y-%m-%d %H:%M:%S") {
	std::time_t t = unix_seconds;
	//struct tm* pt = std::gmtime(&t);
	struct tm* pt = std::localtime(&t);
	if (pt == nullptr) {
		return std::string("");
	}
	uint32_t length = 64;
	std::vector<char> buff(length, '\0');
	strftime(buff.data(), length, format_str.c_str(), pt);
	return std::string(buff.data());
}


struct MyStructDb
{
	bool UpdateToDb() {
		return true;
	}
	bool InsertToDb() {
		return true;
	}
	bool LoadFromDb(std::string data) {
		return true;
	}
	bool DeleteFromDb(int a) {
		return true;
	}
};

template <typename MessageT,
	typename std::enable_if<
	HasDbSerializer<MessageT>::value,
	int>::type = 0>
	bool Meta_UpdateToDb( MessageT& message) {
	return message.UpdateToDb();
}

DEFINE_TYPE_TRAIT(HasTesttest, Testtest)


int main(int argc, char **argv)
{
	MyStructDb db;
	Meta_UpdateToDb(db);

	decltype(&TestA1::Shutdown) varT;

	auto iCurTime = time(NULL);
	std::string formatTime = UnixSecondsToString(iCurTime);
	auto IConvTime = StringToUnixSeconds(formatTime);


	TestA1 testa;

	CallShutdown(&testa);

	Event::Libevent::Global::initialize();
	PlatformImpl platform;

	APie::CtxSingleton::get().init();
	APie::CtxSingleton::get().start();


	::login_msg::MSG_CLIENT_LOGINTOL reqeust;
	reqeust.set_user_id(100);


	std::string msgType = MessageType(reqeust);

	::login_msg::MSG_LOGINSERVER_VALIDATE reqeust1;
	reqeust1.set_port(200);

	::login_msg::MSG_LOGINSERVER_VALIDATE reqeust2;
	reqeust2.set_user_id(1000);

	dispatcher a;
	a.bind(1, fun_1, ::login_msg::MSG_CLIENT_LOGINTOL());
	a.bind(2, fun_2, ::login_msg::MSG_LOGINSERVER_VALIDATE());
	//a.bind(3, fun_3, MyStruct11());

	auto fun1 = std::bind(fun_1, std::placeholders::_1);

	std::string sName = typeid(reqeust).name();

	a.funcs_[1](reqeust);
	a.funcs_[2](reqeust2);
	//a.funcs_[3](reqeust);

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
