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

	class GatewayRole : public std::enable_shared_from_this<GatewayRole>
	{
	public:
		GatewayRole(uint64_t iRoleId, uint64_t iSerialNum);
		~GatewayRole();

		uint64_t getSerailNum();
		uint64_t getRoleId();

		void setMaskFlag(uint32_t iFlag);
		uint32_t getMaskFlag();

		uint64_t addRequestPerUnit(uint64_t iValue);
		void resetRequestPerUnit();

	public:
		static std::shared_ptr<GatewayRole> createGatewayRole(uint64_t iRoleId, uint64_t iSerialNum);

	private:
		void destroy();

	private:
		uint64_t m_iSerialNum = 0;
		uint64_t m_iRoleId = 0;
		uint32_t m_iMaskFlag = 0;

		uint64_t m_iRequests = 0;
		uint64_t m_iRequestPerUnit = 0;
		uint64_t m_iRequestUnitExpiresAt = 0;
	};
}
