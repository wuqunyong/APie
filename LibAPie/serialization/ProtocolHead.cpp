#include "ProtocolHead.h"


ByteBuffer& operator >> (ByteBuffer& stream, ProtocolHead& data)
{
	stream >> data.iMagic;
	stream >> data.iVersion;
	stream >> data.iBodyLen;
	stream >> data.iCheckSum;

	return stream;
}

ByteBuffer& operator << (ByteBuffer& stream, ProtocolHead data)
{
	stream << data.iMagic;
	stream << data.iVersion;
	stream << data.iBodyLen;
	stream << data.iCheckSum;

	return stream;
}
