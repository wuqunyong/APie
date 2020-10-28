#include "gateway_role.h"

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

}

