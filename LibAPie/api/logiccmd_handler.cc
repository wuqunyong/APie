#include "logiccmd_handler.h"

#include <chrono>
#include <string>
#include <iosfwd>
#include <sstream>
#include <algorithm> 

#include "../network/logger.h"


namespace APie {

	LogicCmdHandler::LogicCmdHandler()
	{
		if (!m_init)
		{
			this->registerOnCmd("help", "Display information about builtin commands", LogicCmdHandler::onHelp);
			m_init = true;
		}
	}

	void LogicCmdHandler::init()
	{

	}

	bool LogicCmdHandler::registerOnCmd(const std::string& name, const std::string& desc, Callback cb)
	{
		auto findIte = m_cmd.find(name);
		if (findIte != m_cmd.end())
		{
			ASYNC_PIE_LOG("duplicate|registerOnCmd:%s", PIE_CYCLE_DAY, PIE_ERROR, name.c_str());
			return false;
		}

		cmd_entry_t entry;
		entry.name = name;
		entry.desc = desc;
		entry.cb = cb;

		m_cmd[entry.name] = entry;
		return true;
	}

	std::optional<LogicCmdHandler::Callback> LogicCmdHandler::findCb(const std::string& name)
	{
		auto findIte = m_cmd.find(name);
		if (findIte == m_cmd.end())
		{
			return std::nullopt;
		}

		return findIte->second.cb;
	}

	std::map<std::string, LogicCmdHandler::cmd_entry_t>& LogicCmdHandler::cmds()
	{
		return m_cmd;
	}

	void LogicCmdHandler::onHelp(::pubsub::LOGIC_CMD& cmd)
	{
		std::stringstream ss;

		ss << std::endl;
		for (const auto& items : LogicCmdHandlerSingleton::get().cmds())
		{
			auto iLen = items.second.name.size();
			ss << items.second.name;

			while (iLen < 32)
			{
				ss << " ";
				iLen++;
			}
			ss << "  ";
			ss << items.second.desc << std::endl;
		}

		ASYNC_PIE_LOG("%s", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
		std::cout << ss.str();
	}

} // namespace APie
