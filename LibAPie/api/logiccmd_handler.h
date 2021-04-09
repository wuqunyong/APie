#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <tuple>
#include <mutex>
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"

#include "../../PBMsg/pubsub.pb.h"


namespace APie {


class LogicCmdHandler {
public:
	LogicCmdHandler();

	using Callback = std::function<void(::pubsub::LOGIC_CMD& cmd)>;
	
	struct cmd_entry_t
	{
		std::string name;
		std::string desc;
		Callback cb;
	};

	void init();
	bool registerOnCmd(const std::string& name, const std::string& desc, Callback cb);
	std::optional<Callback> findCb(const std::string& name);

	std::map<std::string, cmd_entry_t>& cmds();

public:
	static void onHelp(::pubsub::LOGIC_CMD& cmd);
	static void onReload(::pubsub::LOGIC_CMD& cmd);

private:
	std::map<std::string, cmd_entry_t> m_cmd;
	bool m_init = false;
};

typedef ThreadSafeSingleton<LogicCmdHandler> LogicCmdHandlerSingleton;



} 
