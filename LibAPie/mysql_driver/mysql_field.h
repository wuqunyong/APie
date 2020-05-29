#pragma once

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <mysql.h>

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

	void setName(char * ptrName, uint32_t len);
	void setType(uint32_t type);
	void setFlags(uint32_t flags);
	void setOffset(uint32_t offset);

	std::string getName();
	uint32_t getType();
	uint32_t getFlags();
	uint32_t getOffset();

	DB_FIELD_TYPE convertType();

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
	std::string m_name;
	uint32_t m_type;
	uint32_t m_flags;
	uint32_t m_offset;
};
