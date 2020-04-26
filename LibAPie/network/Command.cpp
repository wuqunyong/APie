#include "Command.h"

void Envoy::deallocateCommand(Command* cmd)
{
	switch(cmd->type)
	{
	case Command::passive_connect:
	{
		if (NULL != cmd->args.passive_connect.ptrData)
		{
			delete cmd->args.passive_connect.ptrData;
			cmd->args.passive_connect.ptrData = NULL;
		}
		break;
	}
	case Command::pb_reqeust:
	{
		if (NULL != cmd->args.pb_reqeust.ptrData)
		{
			delete cmd->args.pb_reqeust.ptrData;
			cmd->args.pb_reqeust.ptrData = NULL;
		}
		break;
	}
	case Command::send_data:
	{
		if (NULL != cmd->args.send_data.ptrData)
		{
			delete cmd->args.send_data.ptrData;
			cmd->args.send_data.ptrData = NULL;
		}
		break;
	}
	default:
		/*  noop  */;
	}
}
