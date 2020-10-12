#include "dao_factory.h"

namespace APie {

std::map<std::string, DAOFactory::TCreateMethod> DAOFactory::s_methods;

bool DAOFactory::registerFactory(const std::string name, TCreateMethod funcCreate)
{
	if (auto it = s_methods.find(name); it == s_methods.end())
	{
		// C++17 init-if
		s_methods[name] = funcCreate;
		return true;
	}

	return false;
}

std::shared_ptr<DeclarativeBase> DAOFactory::create(const std::string& name)
{
	if (auto it = s_methods.find(name); it != s_methods.end())
	{
		return it->second(); // call the createFunc
	}

	return nullptr;
}

std::map<std::string, DAOFactory::TCreateMethod>& DAOFactory::getMethods()
{
	return s_methods;
}

}