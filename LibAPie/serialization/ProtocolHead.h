#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <string>
#include <vector>
#include <map>

#include "ByteBuffer.h"

/*
Byte order:little-endian
Native byte order is big-endian or little-endian, depending on the host system. For example, Intel x86 and AMD64 (x86-64) are little-endian;
Byte
/       0       |       1       |       2       |       3       |
/               |               |               |               |
|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
+---------------+---------------+---------------+---------------+
0| ---iMagic--- | --iVersion--  |          ---iBodyLen---                 
+---------------+---------------+---------------+---------------+
4|         ---iBodyLen---       |          --iCheckSum---
+---------------+---------------+---------------+---------------+
4|                             Data
+---------------+---------------+---------------+---------------+
4|                             Data
+---------------+---------------+---------------+---------------+
*/

#pragma pack(1)

struct ProtocolHead
{
	ProtocolHead(void)
	{
		this->iMagic = 0x01;
		this->iVersion = 0;
		this->iBodyLen = 0;
		this->iCheckSum = 0;
		//this->iOpcode = 0;
	}

	uint16_t iMagic;
	uint16_t iVersion;
	uint32_t iBodyLen;  
	uint32_t iCheckSum;
	//uint32_t iOpcode;
};

#pragma pack()

extern ByteBuffer& operator >> (ByteBuffer& stream, ProtocolHead& data);
extern ByteBuffer& operator << (ByteBuffer& stream, ProtocolHead data);


