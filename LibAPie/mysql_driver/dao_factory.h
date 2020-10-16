#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>
#include <functional>
#include <optional>

#include "mysql_orm.h"

#include "../singleton/threadsafe_singleton.h"


namespace APie {

	class DAOFactory
	{
	public:
		using TCreateMethod = std::function<std::shared_ptr<DeclarativeBase>()>;

	public:
		bool registerFactory(const std::string name, TCreateMethod funcCreate);
		std::shared_ptr<DeclarativeBase> create(const std::string& name);
		std::map<std::string, TCreateMethod>& getMethods();

		bool addTable(const std::string& name, MysqlTable& table);
		std::optional<MysqlTable> getTable(const std::string& name);

	private:
		std::map<std::string, TCreateMethod> m_methods;
		std::map<std::string, MysqlTable> m_tables;
	};

	class DAOFactoryType
	{
	public:
		enum DBType
		{
			DBT_None = 0,
			DBT_Role = 1,
		};

		bool registerRequiredTable(DBType type, const std::string name, DAOFactory::TCreateMethod funcCreate);
		std::optional<std::map<std::string, DAOFactory::TCreateMethod>> getRequiredTable(DBType type);
		std::shared_ptr<DeclarativeBase> getCreateFunc(DBType type, std::string name);

		bool addLoadedTable(DBType type, const std::string& name, MysqlTable& table);
		DAOFactory* getDAOFactory(DBType type);

	private:
		DAOFactory role;
	};

	typedef ThreadSafeSingleton<DAOFactoryType> DAOFactoryTypeSingleton;
}
