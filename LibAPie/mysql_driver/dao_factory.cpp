#include "dao_factory.h"

namespace APie {


bool DAOFactory::registerFactory(const std::string name, TCreateMethod funcCreate)
{
	if (auto it = m_methods.find(name); it == m_methods.end())
	{
		// C++17 init-if
		m_methods[name] = funcCreate;
		return true;
	}

	return false;
}

std::shared_ptr<DeclarativeBase> DAOFactory::create(const std::string& name)
{
	if (auto it = m_methods.find(name); it != m_methods.end())
	{
		return it->second(); // call the createFunc
	}

	return nullptr;
}

std::map<std::string, DAOFactory::TCreateMethod>& DAOFactory::getMethods()
{
	return m_methods;
}

bool DAOFactory::addTable(const std::string& name, MysqlTable& table)
{
	auto findIte = m_tables.find(name);
	if (findIte != m_tables.end())
	{
		return false;
	}

	m_tables[name] = table;
	return true;
}

std::optional<MysqlTable> DAOFactory::getTable(const std::string& name)
{
	auto findIte = m_tables.find(name);
	if (findIte == m_tables.end())
	{
		return std::nullopt;
	}

	return std::make_optional(findIte->second);
}

bool DAOFactoryType::registerRequiredTable(DBType type, const std::string name, DAOFactory::TCreateMethod funcCreate)
{
	switch (type)
	{
	case APie::DAOFactoryType::DBT_Role:
	{
		return DAOFactoryTypeSingleton::get().role.registerFactory(name, funcCreate);
	}
	default:
	{
		return false;
	}
	}

	return false;
}

std::optional<std::map<std::string, DAOFactory::TCreateMethod>> DAOFactoryType::getRequiredTable(DBType type)
{
	switch (type)
	{
	case APie::DAOFactoryType::DBT_Role:
	{
		return std::make_optional(this->role.getMethods());
	}
	default:
		break;
	}
	return std::nullopt;
}

std::shared_ptr<DeclarativeBase> DAOFactoryType::getCreateFunc(DBType type, std::string name)
{
	switch (type)
	{
	case APie::DAOFactoryType::DBT_Role:
	{
		return this->role.create(name);
	}
	default:
		break;
	}

	return nullptr;
}

bool DAOFactoryType::addLoadedTable(DBType type, const std::string& name, MysqlTable& table)
{
	switch (type)
	{
	case APie::DAOFactoryType::DBT_Role:
	{
		return this->role.addTable(name, table);
	}
	default:
		break;
	}

	return false;
}

DAOFactory* DAOFactoryType::getDAOFactory(DBType type)
{
	switch (type)
	{
	case APie::DAOFactoryType::DBT_Role:
	{
		return &role;
	}
	default:
		break;
	}

	return nullptr;
}

}