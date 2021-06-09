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

	class ModelAccount : public DeclarativeBase {
	public:
		PACKED_STRUCT(struct db_fields {
			uint64_t account_id = 0;
			uint32_t db_id = 0;
			int64_t register_time = 0;
			int64_t modified_time = 0;
		});

		enum Fields
		{
			account_id = 0,
			db_id,
			register_time,
			modified_time,
		};

		virtual void* layoutAddress() override
		{
			return &fields;
		}

		virtual std::vector<uint32_t> layoutOffset() override
		{
			std::vector<uint32_t> layout = {
				offsetof(db_fields, account_id),
				offsetof(db_fields, db_id),
				offsetof(db_fields, register_time),
				offsetof(db_fields, modified_time),
			};

			return layout;
		}

		virtual std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layoutType() override
		{
			std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layout = {
				get_field_type(fields.account_id),
				get_field_type(fields.db_id),
				get_field_type(fields.register_time),
				get_field_type(fields.modified_time),
			};

			return layout;
		}

		static std::shared_ptr<DeclarativeBase> createMethod()
		{
			return std::make_shared<ModelAccount>();
		}

		static std::string getFactoryName() 
		{ 
			return "account"; 
		}

	public:
		db_fields fields;
	};


}
