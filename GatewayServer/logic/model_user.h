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
			uint64_t game_id;
			uint32_t level;
			uint64_t register_time = 1;
			uint64_t login_time = 2;
			uint64_t offline_time = 3;
			std::string name = "hello";
			std::string role_info;
			std::string magic_slot_info;
			std::string magic_info;
			std::string guild_quest;
			std::string match_info;
			std::string global_mails_info;
			std::string treasure_info;
			std::string feats_info;
			std::string small_value;
			uint8_t small_int;
			uint8_t small_int1;
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
				offsetof(db_fields, small_value),
				offsetof(db_fields, small_int),
				offsetof(db_fields, small_int1),
			};

			return layout;
		}

	public:
		db_fields fields;
	};


}
