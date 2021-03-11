#include "gateway_role.h"
#include "gateway_mgr.h"

namespace APie {

std::shared_ptr<GatewayRole> GatewayRole::createGatewayRole(uint64_t iRoleId, uint64_t iSerialNum)
{
	return std::make_shared<GatewayRole>(iRoleId, iSerialNum);
}


GatewayRole::GatewayRole(uint64_t iRoleId, uint64_t iSerialNum) :
	m_iRoleId(iRoleId),
	m_iSerialNum(iSerialNum)
{

}

GatewayRole::~GatewayRole()
{
	this->destroy();
}

void GatewayRole::destroy()
{

}

uint64_t GatewayRole::getSerailNum()
{
	return m_iSerialNum;
}

uint64_t GatewayRole::getRoleId()
{
	return m_iRoleId;
}

void GatewayRole::setMaskFlag(uint32_t iFlag)
{
	m_iMaskFlag = iFlag;
}

uint32_t GatewayRole::getMaskFlag()
{
	return m_iMaskFlag;
}

bool GatewayRole::addRequestPerUnit(uint64_t iValue)
{
	auto iCurTime = APie::CtxSingleton::get().getCurSeconds();
	uint32_t iLimit = APie::CtxSingleton::get().yamlAs<uint32_t>({ "limited", "requests_per_unit" }, 0);

	if (iCurTime > m_iRequestUnitExpiresAt)
	{
		this->resetRequestPerUnit();
	}

	if (m_iRequestUnitExpiresAt == 0)
	{
		uint32_t iDuration = APie::CtxSingleton::get().yamlAs<uint32_t>({ "limited", "uint" }, 60);
		m_iRequestUnitExpiresAt = iCurTime + iDuration;
	}

	m_iRequests += iValue;
	m_iRequestPerUnit += iValue;

	if (m_iRequestPerUnit > iLimit)
	{
		if (iLimit == 0)
		{
			return true;
		}

		std::stringstream ss;
		ss << "recv package out limited|userId:" << m_iRoleId << "|m_iRequestPerUnit:" << m_iRequestPerUnit;
		ASYNC_PIE_LOG("addRequestPerUnit", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

		this->close();
		return false;
	}

	return true;
}

void GatewayRole::resetRequestPerUnit()
{
	m_iRequestPerUnit = 0;
	m_iRequestUnitExpiresAt = 0;
}

void GatewayRole::close()
{
	GatewayMgrSingleton::get().removeGateWayRole(m_iRoleId);
	ServerConnection::sendCloseLocalServer(m_iSerialNum);
}

}

