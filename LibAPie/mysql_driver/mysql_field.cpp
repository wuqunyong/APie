#include "mysql_field.h"


void MysqlField::setIndex(uint32_t index)
{
	this->m_index = index;
}

void MysqlField::setName(char * ptrName, uint32_t len)
{
	this->m_name = std::string(ptrName, len);
}

void MysqlField::setType(uint32_t type)
{
	this->m_type = type;

	m_size = evaluateSize();
}

void MysqlField::setSize(uint32_t size)
{
	this->m_size = size;
}

void MysqlField::setFlags(uint32_t flags)
{
	this->m_flags = flags;
}

void MysqlField::setOffset(uint32_t offset)
{
	this->m_offset = offset;
}

uint32_t MysqlField::getIndex()
{
	return this->m_index;
}

std::string MysqlField::getName()
{
	return this->m_name;
}

uint32_t MysqlField::getType()
{
	return this->m_type;
}

uint32_t MysqlField::getSize()
{
	return this->m_size;
}

uint32_t MysqlField::getFlags()
{
	return this->m_flags;
}

uint32_t MysqlField::getOffset()
{
	return this->m_offset;
}

MysqlField::DB_FIELD_TYPE MysqlField::convertToDbType()
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


::mysql_proxy_msg::MysqlScalarValueTypes MysqlField::convertToPbType()
{
	::mysql_proxy_msg::MysqlScalarValueTypes fieldType;
	switch (m_type)
	{
	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
		fieldType = ::mysql_proxy_msg::MSVT_BYTES;
		break;

	case FIELD_TYPE_STRING:
	case FIELD_TYPE_VAR_STRING:
	case FIELD_TYPE_DATETIME:
		fieldType = ::mysql_proxy_msg::MSVT_STRING;
		break;

	case FIELD_TYPE_FLOAT:
		fieldType = ::mysql_proxy_msg::MSVT_FLOAT;
		break;

	case FIELD_TYPE_DOUBLE:
		fieldType = ::mysql_proxy_msg::MSVT_DOUBLE;// DB_FIELD_TYPE::T_DOUBLE;
		break;

	case FIELD_TYPE_TINY:
		fieldType = ::mysql_proxy_msg::MSVT_INT32;//DB_FIELD_TYPE::T_INT8;
		break;

	case FIELD_TYPE_SHORT:
		fieldType = ::mysql_proxy_msg::MSVT_INT32;//DB_FIELD_TYPE::T_INT16;
		break;

	case FIELD_TYPE_LONG:
		fieldType = ::mysql_proxy_msg::MSVT_INT32;//DB_FIELD_TYPE::T_INT32;
		break;

	case FIELD_TYPE_LONGLONG:
		fieldType = ::mysql_proxy_msg::MSVT_INT64;//DB_FIELD_TYPE::T_INT64;
		break;

	case MYSQL_TYPE_NEWDECIMAL:
		fieldType = ::mysql_proxy_msg::MSVT_INT64;//DB_FIELD_TYPE::T_INT64;
		break;

	default:
		fieldType = ::mysql_proxy_msg::MSVT_None;//DB_FIELD_TYPE::T_NONE;
		break;
	}

	if (fieldType == ::mysql_proxy_msg::MSVT_None)//DB_FIELD_TYPE::T_NONE)
	{
		return fieldType;
	}

	if (is_unsigned())
	{
		switch (fieldType)
		{
		case mysql_proxy_msg::MSVT_INT32:
			fieldType = mysql_proxy_msg::MSVT_UINT32;
			break;
		case mysql_proxy_msg::MSVT_INT64:
			fieldType = mysql_proxy_msg::MSVT_UINT64;
			break;
		default:
			break;
		}
		//fieldType = DB_FIELD_TYPE(uint32_t(fieldType) | 0x20);
	}

	return fieldType;
}

uint32_t MysqlField::evaluateSize()
{
	DB_FIELD_TYPE dbType = convertToDbType();

	switch (dbType)
	{
	case DB_FIELD_TYPE::T_INT8:
	case DB_FIELD_TYPE::T_UINT8:
		return sizeof(int8_t);
	case DB_FIELD_TYPE::T_INT16:
	case DB_FIELD_TYPE::T_UINT16:
		return sizeof(int16_t);
	case DB_FIELD_TYPE::T_INT32:
	case DB_FIELD_TYPE::T_UINT32:
		return sizeof(uint32_t);
	case DB_FIELD_TYPE::T_INT64:
	case DB_FIELD_TYPE::T_UINT64:
		return sizeof(uint64_t);
	case DB_FIELD_TYPE::T_FLOAT:
		return sizeof(float);
	case DB_FIELD_TYPE::T_DOUBLE:
		return sizeof(double);
	case DB_FIELD_TYPE::T_STRING:
	case DB_FIELD_TYPE::T_BYTES:
		return sizeof(std::string);
	default:
		std::stringstream ss;
		ss << "invalid db type" << (uint32_t)dbType;
		throw std::invalid_argument(ss.str());
	}

	return 0;
}