#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"


namespace APie {

	enum Opcodes : uint16_t
	{
		//ҵ������룬��1000��ʼ
		OP_MSG_REQUEST_ACCOUNT_LOGIN_L = 1000,
		OP_MSG_RESPONSE_ACCOUNT_LOGIN_L = 1001,

		OP_MSG_REQUEST_CLIENT_LOGIN = 1002,
		OP_MSG_RESPONSE_CLIENT_LOGIN = 1003,

		OP_MSG_REQUEST_ECHO = 1004,
		OP_MSG_RESPONSE_ECHO = 1005,
	};



	enum ReturnCode : uint32_t
	{
		RC_OK = 0,

		//ҵ�񷵻��룬��1000��ʼ
	};
}
