#include "command.h"

void APie::deallocateCommand(Command* cmd)
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
	case Command::async_log:
	{
		if (NULL != cmd->args.async_log.ptrData)
		{
			delete cmd->args.async_log.ptrData;
			cmd->args.async_log.ptrData = NULL;
		}
		break;
	}
	case Command::metric_data:
	{
		if (NULL != cmd->args.metric_data.ptrData)
		{
			delete cmd->args.metric_data.ptrData;
			cmd->args.metric_data.ptrData = NULL;
		}
		break;
	}
	case Command::dial:
	{
		if (NULL != cmd->args.dial.ptrData)
		{
			delete cmd->args.dial.ptrData;
			cmd->args.dial.ptrData = NULL;
		}
		break;
	}
	case Command::dial_result:
	{
		if (NULL != cmd->args.dial_result.ptrData)
		{
			delete cmd->args.dial_result.ptrData;
			cmd->args.dial_result.ptrData = NULL;
		}
		break;
	}
	case Command::logic_cmd:
	{
		if (NULL != cmd->args.logic_cmd.ptrData)
		{
			delete cmd->args.logic_cmd.ptrData;
			cmd->args.logic_cmd.ptrData = NULL;
		}
		break;
	}
	case Command::close_local_client:
	{
		if (NULL != cmd->args.close_local_client.ptrData)
		{
			delete cmd->args.close_local_client.ptrData;
			cmd->args.close_local_client.ptrData = NULL;
		}
		break;
	}
	case Command::client_peer_close:
	{
		if (NULL != cmd->args.client_peer_close.ptrData)
		{
			delete cmd->args.client_peer_close.ptrData;
			cmd->args.client_peer_close.ptrData = NULL;
		}
		break;
	}
	case Command::server_peer_close:
	{
		if (NULL != cmd->args.server_peer_close.ptrData)
		{
			delete cmd->args.server_peer_close.ptrData;
			cmd->args.server_peer_close.ptrData = NULL;
		}
		break;
	}
	case Command::recv_http_request:
	{
		if (NULL != cmd->args.recv_http_request.ptrData)
		{
			delete cmd->args.recv_http_request.ptrData;
			cmd->args.recv_http_request.ptrData = NULL;
		}
		break;
	}
	case Command::send_http_response:
	{
		if (NULL != cmd->args.send_http_response.ptrData)
		{
			delete cmd->args.send_http_response.ptrData;
			cmd->args.send_http_response.ptrData = NULL;
		}
		break;
	}
	case Command::recv_http_response:
	{
		if (NULL != cmd->args.recv_http_response.ptrData)
		{
			delete cmd->args.recv_http_response.ptrData;
			cmd->args.recv_http_response.ptrData = NULL;
		}
		break;
	}
	default:
		/*  noop  */;
	}
}
