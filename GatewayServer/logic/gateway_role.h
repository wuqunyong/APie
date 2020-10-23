#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>

#include "apie.h"


namespace APie {

	class GatewayRole : std::enable_shared_from_this<GatewayRole>
	{
	public:
		GatewayRole(uint64_t iRoleId, uint64_t iSerialNum);

		uint64_t getSerailNum();
		uint64_t getRoleId();

	public:
		static std::shared_ptr<GatewayRole> createGatewayRole(uint64_t iRoleId, uint64_t iSerialNum);

	private:
		uint64_t m_iSerialNum = 0;
		uint64_t m_iRoleId = 0;

	};
}
