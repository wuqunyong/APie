#include <stdio.h>
#include <iostream>
#include <time.h>

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/port.h>

#include "../PBMsg/login_msg.pb.h"
#include <tuple>

class Base {
public:
	int a;
};

class Dreive1 : public Base
{
public:
	int d;
};

class TestClient {
public:

	using HandleFunction = std::function<void(uint64_t error, ::google::protobuf::Message& ptrMsg)>;

	template <typename F, typename Params, typename std::enable_if<
		std::is_base_of<google::protobuf::Message, Params>::value, int>::type = 0 >
	uint32_t Call(int opcode, F func, Params params)
	{
		index++;
		using OriginType = Params;
		std::string sType = OriginType::descriptor()->full_name();

		if (opcode < 0)
		{
			opcode = 0;
			OriginType data;
			func(opcode, data);
			return index;
		}

		auto ptrCb = [func](uint64_t serialNum, ::google::protobuf::Message& ptrMsg) {
			OriginType data = dynamic_cast<OriginType&>(ptrMsg);
			func(serialNum, data);
		};


		funcs_[index] = ptrCb;
		types_[index] = sType;

		return index;
	}

	uint32_t index = 0;
	std::map<uint64_t, HandleFunction> funcs_;
	std::map<uint64_t, std::string> types_;
};

class MyClass1
{
public:
	static void Func1(uint64_t error, ::login_msg::MSG_Test1& msg)
	{
		std::cout << "error:" << error << ", info:" << msg.DebugString() << std::endl;
	}

	static void Func2(uint64_t error, ::login_msg::MSG_Test2& msg)
	{
		std::cout << "error:" << error << ", info:" << msg.DebugString() << std::endl;
	}
};


int main()
{
	::login_msg::MSG_Test1 msg1;
	msg1.set_id1(100);
	msg1.set_test1_str("hello_100");

	::login_msg::MSG_Test2 msg2;
	msg2.set_id2(200);
	msg2.set_test2_str("hello_200");

	TestClient client;
	client.Call(-10, MyClass1::Func1, ::login_msg::MSG_Test1());

	::login_msg::MSG_Test1 test1;
	int index1 = client.Call(5, MyClass1::Func1, test1);
	int index2 = client.Call(50, MyClass1::Func2, ::login_msg::MSG_Test2());;


	client.funcs_[index1](10, msg1);
	client.funcs_[index2](20, msg2);

	auto ptrTest2 = new ::login_msg::MSG_Test2();
	ptrTest2->set_id2(2000);
	ptrTest2->set_test2_str("hello world");

	google::protobuf::Message* ptrMsg = ptrTest2;
	MyClass1::Func2(1, *ptrTest2);

	uint64_t index = 100;
	std::tuple<uint64_t, google::protobuf::Message &> tData = std::make_tuple(index, std::ref(msg1));

	auto tData2 = tData;

	printf("end main\n");

	getchar();
	return 1;
}