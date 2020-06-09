#include <stdio.h>
#include <iostream>
#include <time.h>
#include <optional>

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/port.h>

#include "../PBMsg/mysql_proxy_msg.pb.h"

#include <tuple>
#include <stdexcept>
#include <iosfwd>
#include <sstream>

#define PACKED_STRUCT(definition, ...)                                                             \
  __pragma(pack(push, 1)) definition, ##__VA_ARGS__;                                               \
  __pragma(pack(pop))


class DeclarativeBase
{
public:
	enum MetaType
	{
		Field_Index = 0,
		Field_Name,
		Field_Type,
		Field_TypeSize,
		Field_Primary,
	};

	//id,name,type,size,primary
	using Column = std::tuple<uint16_t, std::string, ::mysql_proxy_msg::MysqlScalarValueTypes, uint32_t, bool>;
	
	virtual uint32_t blockSize() = 0;
	virtual void* blockAddress() = 0;
	virtual uint32_t columNums() = 0;

	bool query()
	{
		std::vector<uint32_t> primaryIndex;
		uint32_t iIndex = 0;
		for (const auto& items : m_colume)
		{
			bool bResult = std::get<Field_Primary>(items);
			if (bResult)
			{
				primaryIndex.push_back(iIndex);
			}

			iIndex++;
		}

		if (primaryIndex.empty())
		{
			return false;
		}

		return true;
	};

	bool checkInvalid()
	{
		uint32_t iBlockSize = 0;
		for (const auto& items : m_colume)
		{
			auto iSize = std::get<Field_TypeSize>(items);
			iBlockSize += iSize;
		}

		if (iBlockSize != this->blockSize())
		{
			return false;
		}

		if (m_colume.size() != this->columNums())
		{
			return false;
		}

		return true;
	}

	void addColumn(Column info)
	{
		m_colume.push_back(info);
	}

	uint32_t fieldOffset(uint32_t index)
	{
		if (index >= m_colume.size())
		{
			std::stringstream ss;
			ss << "index:" << index << " > size:" << m_colume.size();
			throw std::out_of_range(ss.str().c_str());
		}

		uint32_t offset = 0;
		for (uint32_t i = 0; i < index; i++)
		{
			auto tupleData = m_colume[i];
			offset += std::get<Field_TypeSize>(tupleData);
		}

		return offset;
	}

	uint32_t fieldSize(uint32_t index)
	{
		if (index >= m_colume.size())
		{
			std::stringstream ss;
			ss << "index:" << index << " > size:" << m_colume.size();
			throw std::out_of_range(ss.str().c_str());
		}

		auto tupleData = m_colume[index];
		return std::get<Field_TypeSize>(tupleData);
	}

	template <typename T>
	void Extract(T& t, unsigned char* address)
	{
		t = (*((T*)address));
	};

	std::optional<::mysql_proxy_msg::MysqlValue> getValueByIndex(uint32_t index)
	{
		if (index >= m_colume.size())
		{
			return std::nullopt;
		}

		::mysql_proxy_msg::MysqlValue value;
		void* address = blockAddress();
		uint32_t iOffset = this->fieldOffset(index);
		uint32_t iSize = this->fieldSize(index);

		unsigned char* fieldAddress = (unsigned char*)(address) + iOffset;

		auto tupleData = m_colume[index];
		auto fieldType = std::get<Field_Type>(tupleData);
		value.set_type(fieldType);
		switch (fieldType)
		{
		case ::mysql_proxy_msg::MSVT_INT32:
		{
			uint32_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_INT64:
		{
			int64_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int64_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT32:
		{
			uint32_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_int32_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_UINT64:
		{
			uint64_t fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_uint64_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_BOOL:
		{
			bool fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_bool_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_STRING:
		{
			std::string fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_string_v(fieldValue);
			break;
		}
		case ::mysql_proxy_msg::MSVT_BYTES:
		{
			std::string fieldValue;
			this->Extract(fieldValue, fieldAddress);
			value.set_bytes_v(fieldValue);
			break;

		}
		default:
			return std::nullopt;
		}

		return std::make_optional(value);
	}

private:
	std::string m_tableName;
	std::vector<Column> m_colume;
};


class MySQLData : public DeclarativeBase {
public:
	PACKED_STRUCT(struct db_fields {
		uint32_t id_;
		uint32_t length_;
		std::string name_;
	});

	virtual uint32_t blockSize()
	{
		return sizeof(fields);
	}

	virtual void* blockAddress()
	{
		return &fields;
	}

	virtual uint32_t columNums()
	{
		return 3;
	}

public:
	db_fields fields;
};

template<typename T>
bool query(T data)
{

	return true;
}

int main()
{
	{
		MySQLData data;
		data.addColumn(DeclarativeBase::Column{ 0, std::string("a"), ::mysql_proxy_msg::MSVT_UINT32, (uint32_t)sizeof(uint32_t), true });
		data.addColumn(DeclarativeBase::Column(1, std::string("b"), ::mysql_proxy_msg::MSVT_UINT32, (uint32_t)sizeof(uint32_t), false));
		data.addColumn(DeclarativeBase::Column(uint32_t(2), std::string("c"), ::mysql_proxy_msg::MSVT_STRING, (uint32_t)sizeof(std::string), false));
		bool bResult = data.checkInvalid();

		data.fields.id_ = 123456;
		data.fields.length_ = 100;
		data.fields.name_ = "hello world";

		auto field1 = data.getValueByIndex(0);
		auto field2 = data.getValueByIndex(1);
		auto field3 = data.getValueByIndex(2);
	}


	printf("end main\n");

	getchar();
	return 1;
}