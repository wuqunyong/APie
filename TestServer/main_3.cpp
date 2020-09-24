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
		std::string feats_info;
	});

	virtual void* blockAddress() override
	{
		return &fields;
	}

	virtual uint32_t blockSize() override
	{
		return sizeof(fields);
	}

	virtual std::vector<uint32_t> layoutInfo() override
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
			offsetof(db_fields, feats_info),
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
	PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "startup");

	std::string sPath = APie::Filesystem::Directory::getCWD();
	PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "CurPath:%s", sPath.c_str());

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
		bool bSQL = mysqlConnector.describeTable("role_base", table);
		MySQLData data;
		data.initMetaData(table);
		bResult = data.checkInvalid();

		data.fields.user_id = 200;
		data.fields.game_id = 10980102021;
		std::string querySql = data.query(mysqlConnector);

		std::shared_ptr<ResultSet> recordSet;
		bResult = mysqlConnector.query(querySql.c_str(), querySql.length(), recordSet);
		if (bResult)
		{
			data.loadFromDb(recordSet);
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