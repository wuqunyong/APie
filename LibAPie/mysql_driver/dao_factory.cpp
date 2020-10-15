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

}