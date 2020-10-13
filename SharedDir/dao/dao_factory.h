#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>

#include "apie.h"



namespace APie {

	class DAOFactory
	{
	public:
		using TCreateMethod = std::function<std::shared_ptr<DeclarativeBase>()>;

	public:
		bool registerFactory(const std::string name, TCreateMethod funcCreate);
		std::shared_ptr<DeclarativeBase> create(const std::string& name);
		std::map<std::string, TCreateMethod>& getMethods();

	private:
		std::map<std::string, TCreateMethod> m_methods;
	};

	class DAOFactoryType
	{
	public:
		DAOFactory role;
	};

	typedef ThreadSafeSingleton<DAOFactoryType> DAOFactoryTypeSingleton;
}
