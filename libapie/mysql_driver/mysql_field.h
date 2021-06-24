#pragma once

#include "../network/platform_impl.h"

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <type_traits>

#include <mysql.h>

#include "../pb_msg.h"



class MysqlField
{
public:
	enum class DB_FIELD_TYPE
	{
		T_NONE,

		T_INT8,
		T_INT16,
		T_INT32,
		T_INT64,

		T_FLOAT,
		T_DOUBLE,

		T_STRING,
		T_BYTES,

		T_UINT8 = 0x20 | T_INT8,
		T_UINT16 = 0x20 | T_INT16,
		T_UINT32 = 0x20 | T_INT32,
		T_UINT64 = 0x20 | T_INT64,
	};

	void setIndex(uint32_t index);
	void setName(char * ptrName, uint32_t len);
	void setName(const std::string& name);
	void setType(uint32_t type);
	void setSize(uint32_t size);
	void setFlags(uint32_t flags);
	void setOffset(uint32_t offset);

	uint32_t getIndex();
	std::string getName();
	uint32_t getType();
	uint32_t getSize();
	uint32_t getFlags();
	uint32_t getOffset();

	::mysql_proxy_msg::MysqlValue getValue();
	void setValue(::mysql_proxy_msg::MysqlValue value);

	::mysql_proxy_msg::MysqlScalarValueTypes convertToPbType();
	DB_FIELD_TYPE convertToDbType();

	uint32_t evaluateSize();

	bool is_nullable() const {
		return !(m_flags & NOT_NULL_FLAG);
	}

	bool is_primary_key() const {
		return m_flags & PRI_KEY_FLAG;
	}

	bool is_unique_key() const {
		return m_flags & UNIQUE_KEY_FLAG;
	}

	bool is_multiple_key() const {
		return m_flags & MULTIPLE_KEY_FLAG;
	}

	bool is_unsigned() const {
		return m_flags & UNSIGNED_FLAG;
	}

	bool is_zerofill() const {
		return m_flags & ZEROFILL_FLAG;
	}

	bool is_binary() const {
		return m_flags & BINARY_FLAG;
	}

	bool is_auto_increment() const {
		return m_flags & AUTO_INCREMENT_FLAG;
	}

	bool has_default_value() const {
		return is_nullable() ||
			is_auto_increment() ||
			!(m_flags & NO_DEFAULT_VALUE_FLAG);
	}

private:
	uint32_t m_index;
	std::string m_name;
	uint32_t m_type;
	uint32_t m_size;
	uint32_t m_offset;
	uint32_t m_flags;

	::mysql_proxy_msg::MysqlValue m_value;
};



template <typename T>
std::set<MysqlField::DB_FIELD_TYPE> get_field_type(T t)
{
	std::set<MysqlField::DB_FIELD_TYPE> result;
	if constexpr (std::is_same<T, int8_t>::value) 
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_INT8);
	}
	else if constexpr (std::is_same<T, int16_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_INT16);
	}
	else if constexpr (std::is_same<T, int32_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_INT32);
	}
	else if constexpr (std::is_same<T, int64_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_INT64);
	}
	else if constexpr (std::is_same<T, uint8_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_UINT8);
	}
	else if constexpr (std::is_same<T, uint16_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_UINT16);
	}
	else if constexpr (std::is_same<T, uint32_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_UINT32);
	}
	else if constexpr (std::is_same<T, uint64_t>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_UINT64);
	}
	else if constexpr (std::is_same<T, float>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_FLOAT);
	}
	else if constexpr (std::is_same<T, double>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_DOUBLE);
	}
	else if constexpr (std::is_same<T, std::string>::value)
	{
		result.insert(MysqlField::DB_FIELD_TYPE::T_STRING);
		result.insert(MysqlField::DB_FIELD_TYPE::T_BYTES);
	}

	return result;
}