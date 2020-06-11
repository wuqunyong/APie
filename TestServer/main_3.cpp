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

	std::string query()
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

	std::string toString(::mysql_proxy_msg::MysqlValue& value)
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

	bool loadFromDb(ResultSet* result)
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
				iIndex++;
			}
			//if ((*recordSet >> item.node_id1)
			//	&& (*recordSet >> item.node_id2)
			//	&& (*recordSet >> item.create_time)
			//	&& (*recordSet >> item.update_time)
			//	&& (*recordSet >> item.delete_flag))
			
		}

		if (iRowCount != 1)
		{
			return false;
		}
		return true;
	}
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

		auto fieldType = m_table.getFields()[index].convertToPbType();
		value.set_type(fieldType);
		switch (fieldType)
		{
		case ::mysql_proxy_msg::MSVT_INT32:
		{
			uint32_t fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_int32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_INT64:
		{
			int64_t fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_int64_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT32:
		{
			uint32_t fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_uint32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT64:
		{
			uint64_t fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_uint64_v(fieldValue);
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
		case ::mysql_proxy_msg::MSVT_FLOAT:
		{
			float fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_float_v(fieldValue);
			break;

		}
		case ::mysql_proxy_msg::MSVT_DOUBLE:
		{
			double fieldValue = 0;
			this->Extract(fieldValue, fieldAddress);
			value.set_double_v(fieldValue);
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
		uint16_t level;
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

		data.fields.user_id = 666;
		data.fields.game_id = 10980102021;
		std::string querySql = data.query();

		ResultSet* recordSet = NULL;
		bResult = mysqlConnector.query(querySql.c_str(), querySql.length(), recordSet);
		if (NULL != recordSet)
		{

			delete recordSet;
			recordSet = NULL;
		}


		auto field1 = data.getValueByIndex(0);
		auto field2 = data.getValueByIndex(1);
		auto field3 = data.getValueByIndex(2);
		auto layout = data.layoutInfo();
	}


	printf("end main\n");

	getchar();
	return 1;
}