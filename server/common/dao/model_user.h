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
		});

		enum Fields
		{
			user_id = 0,
			game_id,
			level,
			register_time,
			login_time,
			offline_time,
			name
		};

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
			};

			return layout;
		}

		static std::shared_ptr<DeclarativeBase> createMethod()
		{
			return std::make_shared<ModelUser>();
		}

		static std::string getFactoryName() 
		{ 
			return "role_base"; 
		}

	public:
		db_fields fields;
	};


}
