#include "mysql_orm.h"


bool DeclarativeBase::initMetaData(MysqlTable& table)
{
	m_table = table;
	return true;
}

std::string DeclarativeBase::query()
{
	std::stringstream ss;
	ss << "SELECT ";

	uint32_t iIndex = 0;
	for (auto &items : m_table.getFields())
	{
		iIndex++;
		ss << "`" << items.getName() << "`";

		if (iIndex < m_table.getFields().size())
		{
			ss << ",";
		}
		else
		{
			ss << " ";
		}
	}

	ss << "FROM " << m_table.getTable() << " ";
	ss << "WHERE ";

	bool bFirst = true;
	for (auto& items : m_table.getFields())
	{
		bool bResult = items.is_primary_key();
		if (bResult)
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				ss << " AND ";
			}

			ss << "`" << items.getName() << "`" << "=" << " ";

			std::optional<::mysql_proxy_msg::MysqlValue> field = getValueByIndex(items.getIndex());
			if (field.has_value())
			{
				ss << toString(field.value());
			}
			else
			{
				std::stringstream info;
				ss << "invalid type|index:" << items.getIndex();
				throw std::invalid_argument(info.str());
			}
		}
	}

	if (bFirst)
	{
		std::stringstream info;
		ss << "no primary";
		throw std::invalid_argument(info.str());
	}

	return ss.str();
};

bool DeclarativeBase::checkInvalid()
{
	if (m_table.getFields().size() != this->columNums())
	{
		return false;
	}

	uint32_t iBlockSize = 0;
	for (auto& items : m_table.getFields())
	{
		auto iSize = items.getSize();
		iBlockSize += iSize;
	}

	if (iBlockSize != this->blockSize())
	{
		return false;
	}

	auto layout = this->layoutInfo();
	if (layout.size() != m_table.getFields().size())
	{
		return false;
	}

	uint32_t iIndex = 0;
	for (auto& items : m_table.getFields())
	{
		if (items.getOffset() != layout[iIndex])
		{
			return false;
		}
		++iIndex;
	}

	return true;
}

size_t DeclarativeBase::columNums()
{
	return this->layoutInfo().size();
}

uint32_t DeclarativeBase::fieldOffset(uint32_t index)
{
	if (index >= m_table.getFields().size())
	{
		std::stringstream ss;
		ss << "index:" << index << " > size:" << m_table.getFields().size();
		throw std::out_of_range(ss.str().c_str());
	}

	return m_table.getFields()[index].getOffset();
}

uint32_t DeclarativeBase::fieldSize(uint32_t index)
{
	if (index >= m_table.getFields().size())
	{
		std::stringstream ss;
		ss << "index:" << index << " > size:" << m_table.getFields().size();
		throw std::out_of_range(ss.str().c_str());
	}

	return m_table.getFields()[index].getSize();
}

std::string DeclarativeBase::toString(::mysql_proxy_msg::MysqlValue& value)
{
	std::string quotes("'");

	std::stringstream ss;
	switch (value.type())
	{
	case ::mysql_proxy_msg::MSVT_INT32:
	{
		ss << value.int32_v();
		break;
	}
	case ::mysql_proxy_msg::MSVT_INT64:
	{
		ss << value.int64_v();
		break;
	}
	case ::mysql_proxy_msg::MSVT_UINT32:
	{
		ss << value.uint32_v();
		break;
	}
	case ::mysql_proxy_msg::MSVT_UINT64:
	{
		ss << value.uint64_v();
		break;
	}
	case ::mysql_proxy_msg::MSVT_STRING:
	{
		ss << quotes << value.string_v() << quotes;
		break;
	}
	case ::mysql_proxy_msg::MSVT_BYTES:
	{
		ss << quotes << value.bytes_v() << quotes;
		break;

	}
	case ::mysql_proxy_msg::MSVT_FLOAT:
	{
		ss << value.float_v();
		break;

	}
	case ::mysql_proxy_msg::MSVT_DOUBLE:
	{
		ss << value.double_v();
		break;

	}
	default:
		break;
	}

	return ss.str();
}

bool DeclarativeBase::loadFromDb(ResultSet* result)
{
	if (result == nullptr)
	{
		return false;
	}

	uint32_t iRowCount = 0;
	while (result->MoveNext())
	{
		iRowCount++;

		uint32_t iIndex = 0;
		for (auto &items : m_table.getFields())
		{
			void* address = blockAddress();
			uint32_t iOffset = this->fieldOffset(iIndex);

			switch (items.convertToDbType())
			{
			case MysqlField::DB_FIELD_TYPE::T_INT8:
			{
				int8_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);

				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_INT16:
			{
				int16_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_INT32:
			{
				int32_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_INT64:
			{
				int64_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_FLOAT:
			{
				float fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_DOUBLE:
			{
				double fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_BYTES:
			{
				std::string fieldValue;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_STRING:
			{
				std::string fieldValue;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_UINT8:
			{
				uint8_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_UINT16:
			{
				uint16_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_UINT32:
			{
				uint32_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			case MysqlField::DB_FIELD_TYPE::T_UINT64:
			{
				uint64_t fieldValue = 0;
				*result >> fieldValue;

				unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;
				this->writeValue(fieldAddress, fieldValue);
				break;
			}
			default:
				break;
			}

			iIndex++;
		}

	}

	if (iRowCount != 1)
	{
		return false;
	}
	return true;
}

std::optional<::mysql_proxy_msg::MysqlValue> DeclarativeBase::getValueByIndex(uint32_t index)
{
	if (index >= m_table.getFields().size())
	{
		return std::nullopt;
	}

	::mysql_proxy_msg::MysqlValue value;
	void* address = blockAddress();
	uint32_t iOffset = this->fieldOffset(index);

	unsigned char* fieldAddress = (unsigned char*)(address)+iOffset;

	auto fieldType = m_table.getFields()[index].convertToPbType();
	value.set_type(fieldType);
	switch (fieldType)
	{
	case ::mysql_proxy_msg::MSVT_INT32:
	{
		uint32_t fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_int32_v(fieldValue);
		break;
	}
	case ::mysql_proxy_msg::MSVT_INT64:
	{
		int64_t fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_int64_v(fieldValue);
		break;
	}
	case ::mysql_proxy_msg::MSVT_UINT32:
	{
		uint32_t fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_uint32_v(fieldValue);
		break;
	}
	case ::mysql_proxy_msg::MSVT_UINT64:
	{
		uint64_t fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_uint64_v(fieldValue);
		break;
	}
	case ::mysql_proxy_msg::MSVT_STRING:
	{
		std::string fieldValue;
		this->extract(fieldValue, fieldAddress);
		value.set_string_v(fieldValue);
		break;
	}
	case ::mysql_proxy_msg::MSVT_BYTES:
	{
		std::string fieldValue;
		this->extract(fieldValue, fieldAddress);
		value.set_bytes_v(fieldValue);
		break;

	}
	case ::mysql_proxy_msg::MSVT_FLOAT:
	{
		float fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_float_v(fieldValue);
		break;

	}
	case ::mysql_proxy_msg::MSVT_DOUBLE:
	{
		double fieldValue = 0;
		this->extract(fieldValue, fieldAddress);
		value.set_double_v(fieldValue);
		break;
	}
	default:
		return std::nullopt;
	}

	return std::make_optional(value);
}

