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


#ifdef _MSC_VER
#define PACKED_STRUCT(definition, ...)                                                             \
  __pragma(pack(push, 1)) definition, ##__VA_ARGS__;                                               \
  __pragma(pack(pop))
#else
#define PACKED_STRUCT(definition, ...) definition, ##__VA_ARGS__
#endif


//#define PACKED_STRUCT(definition, ...) definition, ##__VA_ARGS__ __attribute__((packed))

class DeclarativeBase
{
public:

	//virtual uint32_t blockSize() = 0;
	virtual void* layoutAddress() = 0;
	virtual std::vector<uint32_t> layoutOffset() = 0;
	virtual std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layoutType() = 0;

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

	bool initMetaData(MysqlTable& table);
	bool bindTable(uint32_t type, const std::string& name);

	bool loadFromDb(std::shared_ptr<ResultSet> sharedPtr);
	bool loadFromPb(const ::mysql_proxy_msg::MysqlRow& row);

	bool loadFromPbCheck(::mysql_proxy_msg::MysqlQueryResponse& response);

	bool checkInvalid();

	std::optional<::mysql_proxy_msg::MysqlValue> getValueByIndex(uint32_t index);
	uint32_t getLayoutOffset(uint32_t index);

	size_t columNums();
	uint32_t fieldSize(uint32_t index);
	uint32_t getRowCount();

	void markDirty(const std::vector<uint8_t>& index);
	bool isDirty(uint8_t index);
	void dirtySet();
	void dirtyReset();

	void markFilter(const std::vector<uint8_t>& index);
	bool isFilter(uint8_t index);
	void filterReset();

	std::string query(MySQLConnector& connector);

	mysql_proxy_msg::MysqlQueryRequest generateQuery();
	mysql_proxy_msg::MysqlInsertRequest generateInsert();
	mysql_proxy_msg::MysqlUpdateRequest generateUpdate();
	mysql_proxy_msg::MysqlDeleteRequest generateDelete();

	mysql_proxy_msg::MysqlQueryRequestByFilter generateQueryByFilter();

private:
	bool loadFromPb(::mysql_proxy_msg::MysqlQueryResponse& response);

public:
	static std::string toString(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlValue& value);

	static MysqlTable convertFrom(::mysql_proxy_msg::MysqlDescTable& desc);
	static mysql_proxy_msg::MysqlQueryResponse convertFrom(MysqlTable& table, std::shared_ptr<ResultSet> sharedPtr);
	static std::optional<mysql_proxy_msg::MysqlRow> convertToRowFrom(MysqlTable& table, std::shared_ptr<ResultSet> sharedPtr);


private:
	MysqlTable m_table;
	std::bitset<256> m_dirtyFlags;
	std::bitset<256> m_filterFlags;
	uint32_t m_rowCount = 0;
};

