#include "logiccmd_handler.h"

#include <chrono>
#include <string>
#include <iosfwd>
#include <sstream>
#include <algorithm> 

#include "../network/logger.h"
#include "../common/file.h"


namespace APie {

	LogicCmdHandler::LogicCmdHandler()
	{
		if (!m_init)
		{
			this->registerOnCmd("help", "Display information about builtin commands", LogicCmdHandler::onHelp);
			this->registerOnCmd("reload", "Reload the configuration file", LogicCmdHandler::onReload);
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

	void LogicCmdHandler::onReload(::pubsub::LOGIC_CMD& cmd)
	{
		std::string configFile;
		try {
			configFile = APie::CtxSingleton::get().getConfigFile();

			int64_t mtime = APie::Common::FileDataModificationTime(configFile);
			if (mtime == -1)
			{
				PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "reload|configFile:%s not exist", configFile.c_str());
				return;
			}

			if (APie::CtxSingleton::get().getConfigFileMTime() == mtime)
			{
				PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_NOTICE, "reload|configFile:%s not changed", configFile.c_str());
				return;
			}

			auto node = YAML::LoadFile(configFile);
			APie::CtxSingleton::get().resetYamlNode(node);
			APie::CtxSingleton::get().setConfigFileMTime(mtime);

			PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_NOTICE, "reload|configFile:%s changed", configFile.c_str());
		}
		catch (std::exception& e) {
			std::stringstream ss;
			ss << "reload|fileName:" << configFile << "|Unexpected exception: " << e.what();
			PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		}
	}

} // namespace APie
