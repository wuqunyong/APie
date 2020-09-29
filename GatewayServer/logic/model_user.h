#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"


namespace APie {

	class ModelUser : public DeclarativeBase {
	public:
		PACKED_STRUCT(struct db_fields {
			uint64_t user_id;
			uint64_t game_id = 1;
			uint32_t level = 2;
			int64_t register_time = 1;
			int64_t login_time = 2;
			int64_t offline_time = 3;
			std::string name = "hello";
			std::string role_info;
			std::string magic_slot_info;
			std::string magic_info;
			std::string guild_quest;
			std::string match_info;
			std::string global_mails_info;
			std::string treasure_info;
			std::string feats_info;
			std::string small_value = "2020-09-23 17:24:20";
			int8_t small_int = 98;
			int8_t small_int1 = 123;
		});

		virtual void* layoutAddress() override
		{
			return &fields;
		}

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
				offsetof(db_fields, feats_info),
				offsetof(db_fields, small_value),
				offsetof(db_fields, small_int),
				offsetof(db_fields, small_int1),
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
				get_field_type(fields.feats_info),
				get_field_type(fields.small_value),
				get_field_type(fields.small_int),
				get_field_type(fields.small_int1)
			};

			return layout;
		}

	public:
		db_fields fields;
	};


}
