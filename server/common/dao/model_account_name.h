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

	class ModelAccountName : public DeclarativeBase {
	public:
		PACKED_STRUCT(struct db_fields {
			uint64_t account_id = 0;
			std::string name;
		});

		enum Fields
		{
			account_id = 0,
			name,
		};

		virtual void* layoutAddress() override
		{
			return &fields;
		}

		virtual std::vector<uint32_t> layoutOffset() override
		{
			std::vector<uint32_t> layout = {
				offsetof(db_fields, account_id),
				offsetof(db_fields, name),
			};

			return layout;
		}

		virtual std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layoutType() override
		{
			std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layout = {
				get_field_type(fields.account_id),
				get_field_type(fields.name),
			};

			return layout;
		}

		static std::shared_ptr<DeclarativeBase> createMethod()
		{
			return std::make_shared<ModelAccountName>();
		}

		static std::string getFactoryName() 
		{ 
			return "account_name"; 
		}

	public:
		db_fields fields;
	};


}
