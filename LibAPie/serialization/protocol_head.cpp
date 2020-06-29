#include "protocol_head.h"


ByteBuffer& operator >> (ByteBuffer& stream, ProtocolHead& data)
{
	stream >> data.iMagic;
	stream >> data.iOpcode;
	stream >> data.iBodyLen;
	stream >> data.iCheckSum;

	return stream;
}

ByteBuffer& operator << (ByteBuffer& stream, ProtocolHead data)
{
	stream << data.iMagic;
	stream << data.iOpcode;
	stream << data.iBodyLen;
	stream << data.iCheckSum;

	return stream;
}
