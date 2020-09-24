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
#include <bitset>
#include <vector>

#include <google/protobuf/message.h>

#include "mysql_table.h"
#include "mysql_connector.h"
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

	uint32_t getRowCount();

	bool loadFromDb(std::shared_ptr<ResultSet> sharedPtr);
	bool loadFromPb(::mysql_proxy_msg::MysqlQueryResponse& response);

	std::optional<::mysql_proxy_msg::MysqlValue> getValueByIndex(uint32_t index);

	bool checkInvalid();

	size_t columNums();
	uint32_t fieldOffset(uint32_t index);
	uint32_t fieldSize(uint32_t index);

	std::string query(MySQLConnector& connector);
	void markDirty(const std::vector<uint8_t>& index);
	bool isDirty(uint8_t index);
	void dirtySet();
	void dirtyReset();

	mysql_proxy_msg::MysqlQueryRequest generateQuery();
	mysql_proxy_msg::MysqlInsertRequest generateInsert();
	mysql_proxy_msg::MysqlUpdateRequest generateUpdate();

public:
	static MysqlTable convertFrom(::mysql_proxy_msg::MysqlDescTable& desc);
	static std::string toString(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlValue& value);
	static mysql_proxy_msg::MysqlQueryResponse convertFrom(MysqlTable& table, std::shared_ptr<ResultSet> sharedPtr);

private:
	MysqlTable m_table;
	std::bitset<256> m_dirtyFlags;
	uint32_t m_rowCount = 0;
};

