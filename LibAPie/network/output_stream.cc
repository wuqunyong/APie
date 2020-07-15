#include "../network/output_stream.h"


#include <array>
#include <cstdint>
#include <string>
#include <assert.h>

#include "../event/dispatcher_impl.h"
#include "../network/ctx.h"
#include "../serialization/protocol_head.h"

namespace APie {
namespace Network {

	bool OutputStream::sendMsg(uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg, ConnetionType type)
	{
		uint32_t iThreadId = 0;
		
		switch (type)
		{
		case APie::ConnetionType::CT_NONE:
		{
			auto ptrConnection = Event::DispatcherImpl::getConnection(iSerialNum);
			if (ptrConnection == nullptr)
			{
				auto ptrClient = Event::DispatcherImpl::getClientConnection(iSerialNum);
				if (ptrClient == nullptr)
				{
					return false;
				}
				else
				{
					iThreadId = ptrClient->getTId();
					type = ConnetionType::CT_CLIENT;
				}
			}
			else
			{
				iThreadId = ptrConnection->getTId();
				type = ConnetionType::CT_SERVER;
			}
			break;
		}
		case APie::ConnetionType::CT_SERVER:
		{
			auto ptrConnection = Event::DispatcherImpl::getConnection(iSerialNum);
			if (ptrConnection == nullptr)
			{
				return false;
			}

			iThreadId = ptrConnection->getTId();
			break;
		}
		case APie::ConnetionType::CT_CLIENT:
		{
			auto ptrConnection = Event::DispatcherImpl::getClientConnection(iSerialNum);
			if (ptrConnection == nullptr)
			{
				return false;
			}

			iThreadId = ptrConnection->getTId();
			break;
		}
		default:
			break;
		}

		auto ptrThread = CtxSingleton::get().getThreadById(iThreadId);
		if (ptrThread == nullptr)
		{
			return false;
		}

		ProtocolHead head;
		head.iOpcode = iOpcode;
		head.iBodyLen = (uint32_t)msg.ByteSizeLong();

		//size_t iSize = sizeof(ProtocolHead) + head.iBodyLen;

		SendData *itemObjPtr = new SendData;
		itemObjPtr->type = type;
		itemObjPtr->iSerialNum = iSerialNum;
		itemObjPtr->sData.append(reinterpret_cast<char*>(&head), sizeof(ProtocolHead));
		itemObjPtr->sData.append(msg.SerializeAsString());

		Command command;
		command.type = Command::send_data;
		command.args.send_data.ptrData = itemObjPtr;
		ptrThread->push(command);

		return true;
	}

} // namespace Network
} // namespace Envoy
