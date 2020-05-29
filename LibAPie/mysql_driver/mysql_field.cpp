#include "mysql_field.h"


void MysqlField::setName(char * ptrName, uint32_t len)
{
	this->m_name = std::string(ptrName, len);
}

void MysqlField::setType(uint32_t type)
{
	this->m_type = type;
}

void MysqlField::setFlags(uint32_t flags)
{
	this->m_flags = flags;
}

void MysqlField::setOffset(uint32_t offset)
{
	this->m_offset = offset;
}

std::string MysqlField::getName()
{
	return this->m_name;
}

uint32_t MysqlField::getType()
{
	return this->m_type;
}

uint32_t MysqlField::getFlags()
{
	return this->m_flags;
}

uint32_t MysqlField::getOffset()
{
	return this->m_offset;
}