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

template<class... Ts>
struct PersistModelType
{
	using tuple_t = decltype(std::make_tuple(std::declval<Ts>()...));
};


class MySQLData : public DeclarativeBase {
public:
	PACKED_STRUCT(struct db_fields {
		uint64_t user_id;
		uint64_t game_id;
		uint32_t level;
		int64_t register_time;
		int64_t login_time;
		int64_t offline_time;
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

	virtual void* layoutAddress() override
	{
		return &fields;
	}

	//virtual uint32_t blockSize() override
	//{
	//	return sizeof(fields);
	//}

	virtual std::vector<uint32_t> layoutOffset() override
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
			offsetof(db_fields, feats_info)
		};

		return layout;
	}

	virtual std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layoutType() override
	{
		std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layout = {
			get_field_type(fields.user_id),
			get_field_type(fields.game_id),
			get_field_type(fields.level),
			get_field_type(fields.register_time),
			get_field_type(fields.login_time),
			get_field_type(fields.offline_time),
			get_field_type(fields.name),
			get_field_type(fields.role_info),
			get_field_type(fields.magic_slot_info),
			get_field_type(fields.magic_info),
			get_field_type(fields.guild_quest),
			get_field_type(fields.match_info),
			get_field_type(fields.global_mails_info),
			get_field_type(fields.treasure_info),
			get_field_type(fields.feats_info)
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

	PersistModelType<uint32_t, std::string, uint64_t>::tuple_t stub1 = { 1,"hello", 100 };
	std::cout << std::get<0>(stub1) << "," << std::get<1>(stub1) << "," << std::get<2>(stub1);
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
		auto layout = data.layoutOffset();
	}


	printf("end main\n");

	getchar();
	return 1;
}