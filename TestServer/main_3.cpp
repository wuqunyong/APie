#include <stdio.h>
#include <iostream>
#include <time.h>
#include <optional>

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/port.h>

#include "apie.h"
#include "../PBMsg/mysql_proxy_msg.pb.h"

#include <tuple>
#include <stdexcept>
#include <iosfwd>
#include <sstream>

#define PACKED_STRUCT(definition, ...)                                                             \
  __pragma(pack(push, 1)) definition, ##__VA_ARGS__;                                               \
  __pragma(pack(pop))


class DeclarativeBase
{
public:

	virtual uint32_t blockSize() = 0;
	virtual void* blockAddress() = 0;
	virtual std::vector<uint32_t> layoutInfo() = 0;

	bool InitMetaData(MysqlTable& table)
	{
		m_table = table;
		return true;
	}

	bool query()
	{
		std::vector<uint32_t> primaryIndex;
		uint32_t iIndex = 0;
		for (const auto& items : m_table.getFields())
		{
			bool bResult = items.is_primary_key();
			if (bResult)
			{
				primaryIndex.push_back(iIndex);
			}

			iIndex++;
		}

		if (primaryIndex.empty())
		{
			return false;
		}

		return true;
	};

	bool checkInvalid()
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

	size_t columNums()
	{
		return this->layoutInfo().size();
	}

	uint32_t fieldOffset(uint32_t index)
	{
		if (index >= m_table.getFields().size())
		{
			std::stringstream ss;
			ss << "index:" << index << " > size:" << m_table.getFields().size();
			throw std::out_of_range(ss.str().c_str());
		}

		return m_table.getFields()[index].getOffset();
	}

	uint32_t fieldSize(uint32_t index)
	{
		if (index >= m_table.getFields().size())
		{
			std::stringstream ss;
			ss << "index:" << index << " > size:" << m_table.getFields().size();
			throw std::out_of_range(ss.str().c_str());
		}

		return m_table.getFields()[index].getSize();
	}

	template <typename T>
	void Extract(T& t, unsigned char* address)
	{
		t = (*((T*)address));
	};

	std::optional<::mysql_proxy_msg::MysqlValue> getValueByIndex(uint32_t index)
	{
		if (index >= m_table.getFields().size())
		{
			return std::nullopt;
		}

		::mysql_proxy_msg::MysqlValue value;
		void* address = blockAddress();
		uint32_t iOffset = this->fieldOffset(index);
		uint32_t iSize = this->fieldSize(index);

		unsigned char* fieldAddress = (unsigned char*)(address) + iOffset;

		auto fieldType = m_table.getFields()[index].getScalarType();
		value.set_type(fieldType);
		switch (fieldType)
		{
		case ::mysql_proxy_msg::MSVT_INT32:
		{
			uint32_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_INT64:
		{
			int64_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int64_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT32:
		{
			uint32_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT64:
		{
			uint64_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_uint64_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_BOOL:
		{
			bool fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_bool_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_STRING:
		{
			std::string fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_string_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_BYTES:
		{
			std::string fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_bytes_v(fieldValue);
			break;

		}
		default:
			return std::nullopt;
		}

		return std::make_optional(value);
	}

private:
	MysqlTable m_table;
};


class MySQLData : public DeclarativeBase {
public:
	PACKED_STRUCT(struct db_fields {
		uint64_t user_id;
		uint64_t game_id;
		uint32_t level;
		uint64_t register_time;
		uint64_t login_time;
		uint64_t offline_time;
		std::string name;
		std::string role_info;
		std::string magic_slot_info;
		std::string magic_info;
		std::string guild_quest;
		std::string match_info;
		std::string global_mails_info;
		std::string treasure_info;
	});

	virtual void* blockAddress()
	{
		return &fields;
	}

	virtual uint32_t blockSize()
	{
		return sizeof(fields);
	}

	virtual std::vector<uint32_t> layoutInfo()
	{
		std::vector<uint32_t> layout = {
			offsetof(db_fields, user_id),
			offsetof(db_fields, game_id),
			offsetof(db_fields, level),
			offsetof(db_fields, register_time),
			offsetof(db_fields, login_time),
			offsetof(db_fields, offline_time),
			offsetof(db_fields, name),
			offsetof(db_fields, role_info),
			offsetof(db_fields, magic_slot_info),
			offsetof(db_fields, magic_info),
			offsetof(db_fields, guild_quest),
			offsetof(db_fields, match_info),
			offsetof(db_fields, global_mails_info),
			offsetof(db_fields, treasure_info),
		};

		return layout;
	}

public:
	db_fields fields;
};

template<typename T>
bool query(T data)
{

	return true;
}


PACKED_STRUCT(struct test_fields {
	uint32_t id_;
	uint32_t length_;
	std::string name_;
} aa, bb, cc);

int main()
{
	bb.id_ = 10;
	cc.length_ = 20;
	aa.name_ = "hello";

	{

		MySQLConnectOptions options;
		options.host = "127.0.0.1";
		options.user = "root";
		options.passwd = "root";
		options.db = "ff_base1";
		options.port = 3306;

		MySQLConnector mysqlConnector;
		mysqlConnector.init(options);
		bool bResult = mysqlConnector.connect();

		if (!bResult)
		{
			std::stringstream ss;
			ss << "DbThread::init mysql_connector connect error, ";
		}

		std::string sql("SELECT * FROM role_base WHERE FALSE;");

		MysqlTable table;
		bool bSQL = mysqlConnector.describeTable("role_base_copy", table);
		MySQLData data;
		data.InitMetaData(table);
		bResult = data.checkInvalid();


		auto field1 = data.getValueByIndex(0);
		auto field2 = data.getValueByIndex(1);
		auto field3 = data.getValueByIndex(2);
		auto layout = data.layoutInfo();
	}


	printf("end main\n");

	getchar();
	return 1;
}