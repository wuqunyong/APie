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

MysqlField::DB_FIELD_TYPE MysqlField::convertType()
{
	DB_FIELD_TYPE fieldType;
	switch (m_type)
	{
	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
		fieldType = DB_FIELD_TYPE::T_BYTES;
		break;

	case FIELD_TYPE_STRING:
	case FIELD_TYPE_VAR_STRING:
	case FIELD_TYPE_DATETIME:
		fieldType = DB_FIELD_TYPE::T_STRING;
		break;

	case FIELD_TYPE_FLOAT:
		fieldType = DB_FIELD_TYPE::T_FLOAT;
		break;

	case FIELD_TYPE_DOUBLE:
		fieldType = DB_FIELD_TYPE::T_DOUBLE;
		break;

	case FIELD_TYPE_TINY:
		fieldType = DB_FIELD_TYPE::T_INT8;
		break;

	case FIELD_TYPE_SHORT:
		fieldType = DB_FIELD_TYPE::T_INT16;
		break;

	case FIELD_TYPE_LONG:
		fieldType = DB_FIELD_TYPE::T_INT32;
		break;

	case FIELD_TYPE_LONGLONG:
		fieldType = DB_FIELD_TYPE::T_INT64;
		break;

	case MYSQL_TYPE_NEWDECIMAL:
		fieldType = DB_FIELD_TYPE::T_INT64;
		break;

	default:
		fieldType = DB_FIELD_TYPE::T_NONE;
		break;
	}

	if (fieldType == DB_FIELD_TYPE::T_NONE)
	{
		return fieldType;
	}

	if (is_unsigned())
	{
		fieldType = DB_FIELD_TYPE(uint32_t(fieldType) | 0x20);
	}

	return fieldType;
}