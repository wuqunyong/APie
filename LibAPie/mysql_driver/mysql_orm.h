#pragma once

#include "../network/windows_platform.h"

#include <stdio.h>
#include <time.h>

#include <iostream>
#include <optional>
#include <tuple>
#include <stdexcept>
#include <iosfwd>
#include <sstream>

#include <google/protobuf/message.h>

#include "mysql_table.h"
#include "result_set.h"

#include "../../PBMsg/mysql_proxy_msg.pb.h"


#define PACKED_STRUCT(definition, ...)                                                             \
  __pragma(pack(push, 1)) definition, ##__VA_ARGS__;                                               \
  __pragma(pack(pop))


class DeclarativeBase
{
public:

	virtual uint32_t blockSize() = 0;
	virtual void* blockAddress() = 0;
	virtual std::vector<uint32_t> layoutInfo() = 0;

	bool initMetaData(MysqlTable& table);

	template <typename T>
	void extract(T& t, unsigned char* address)
	{
		t = (*((T*)address));
	};

	template <typename T>
	void writeValue(void* address, T value)
	{
		*((T*)address) = value;
	}

	bool loadFromDb(std::shared_ptr<ResultSet> sharedPtr);
	std::optional<::mysql_proxy_msg::MysqlValue> getValueByIndex(uint32_t index);
	std::string toString(::mysql_proxy_msg::MysqlValue& value);

	bool checkInvalid();

	size_t columNums();
	uint32_t fieldOffset(uint32_t index);
	uint32_t fieldSize(uint32_t index);

	std::string query();

private:
	MysqlTable m_table;
};

