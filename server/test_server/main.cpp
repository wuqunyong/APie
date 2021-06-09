#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <locale.h>

#include <openssl/des.h>

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

	using HandleFunction = std::function<void(::google::protobuf::Message& msg)>;
	std::map<uint64_t, HandleFunction> funcs_;
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
	enum MyEnum
	{
		Field_0 = 0,
		Field_1,
		Field_2
	};

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

	int a;
	int b;

	std::string c;

	static std::string d;
};

std::string MyStructDb::d;


template <typename MessageT,
	typename std::enable_if<
	HasDbSerializer<MessageT>::value,
	int>::type = 0>
	bool Meta_UpdateToDb( MessageT& message) {
	return message.UpdateToDb();
}

DEFINE_TYPE_TRAIT(HasTesttest, Testtest)



void test_crypto()
{
	DES_cblock key;
	//随机密钥  
	DES_random_key(&key);

	DES_key_schedule schedule;
	//转换成schedule  
	DES_set_key_checked(&key, &schedule);

	const_DES_cblock input = "hehehe";
	DES_cblock output;

	printf("cleartext: %s\n", input);

	//加密  
	DES_ecb_encrypt(&input, &output, &schedule, DES_ENCRYPT);
	printf("Encrypted!\n");

	printf("ciphertext: ");
	int i;
	for (i = 0; i < sizeof(input); i++)
		printf("%02x", output[i]);
	printf("\n");

	//解密  
	DES_ecb_encrypt(&output, &input, &schedule, DES_DECRYPT);
	printf("Decrypted!\n");
	printf("cleartext:%s\n", input);

}

class TestPbClass
{
public:
	static void HandleFun1(uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << std::endl;
		msg.PrintDebugString();


		::login_msg::MSG_CLIENT_LOGINTOL response;
		response.set_user_id(msg.user_id());

		Network::OutputStream::sendMsg(serialNum, 1101, response);
	};
};

int main(int argc, char **argv)
{
	std::string strMds = APie::Crypto::Utility::md5("hello");

	test_crypto();

	int bPodS = sizeof(MyStructDb);

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

	auto ptrCb = [](uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << std::endl;
		msg.PrintDebugString();


		::login_msg::MSG_CLIENT_LOGINTOL response;
		response.set_user_id(msg.user_id());

		Network::OutputStream::sendMsg(serialNum, 1101, response);
	};
	Api::PBHandlerSingleton::get().bind(1100, TestPbClass::HandleFun1, ::login_msg::MSG_CLIENT_LOGINTOL());





	MySQLConnectOptions options;
	options.host = "127.0.0.1";
	options.user = "root";
	options.passwd = "root";
	options.db = "ff_base1";
	options.port = 3306;

	MySQLConnector mysqlConnector;
	mysqlConnector.init(options);
	bool bResult = mysqlConnector.connect();
	if (!bResult)
	{
		std::stringstream ss;
		ss << "DbThread::init mysql_connector connect error, " ;
	}

	std::string sql("SELECT * FROM role_base WHERE FALSE;");

	ResultSet* results = nullptr;
	bool bSQL = mysqlConnector.query(sql.c_str(), sql.size(), results);


	MysqlTable table;
	bSQL = mysqlConnector.describeTable("role_base", table);

	std::cin.get();

    return 0;
}
