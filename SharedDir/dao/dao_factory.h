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
		DAOFactory() = delete;

		static bool registerFactory(const std::string name, TCreateMethod funcCreate);
		static std::shared_ptr<DeclarativeBase> create(const std::string& name);
		static std::map<std::string, TCreateMethod>& getMethods();

	private:
		static std::map<std::string, TCreateMethod> s_methods;
	};


}
