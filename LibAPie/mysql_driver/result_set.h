#pragma once

#ifdef WIN32
#include "../network/windows_platform.h"
#else
#include <pthread.h>
#endif

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include <mysql.h>


class ResultSet
{
	ResultSet(const ResultSet& rhs);
	ResultSet& operator=(const ResultSet& rhs);

public:
	ResultSet(MYSQL_RES* ptr_mysql_res);
	virtual ~ResultSet();

	bool MoveNext();

	bool operator>> (int8_t& ref_value);
	bool operator>> (int16_t& ref_value);
	bool operator>> (int32_t& ref_value);
	bool operator>> (int64_t& ref_value);

	bool operator>> (uint8_t& ref_value);
	bool operator>> (uint16_t& ref_value);
	bool operator>> (uint32_t& ref_value);
	bool operator>> (uint64_t& ref_value);

	bool operator>> (float& ref_value);
	bool operator>> (double& ref_value);


	bool operator>> (std::string& ref_value)
	{
		if ( (NULL == this->mysql_rows_) || (NULL == this->lengths_) || (this->index_ >= this->num_fields_) )
		{
			return false;
		}

        uint32_t cur_index = this->index_;

		const char* ptr_field = this->mysql_rows_[this->index_++];
		if (NULL == ptr_field)
		{
			return false;
		}

        unsigned long binary_len = lengths_[cur_index];

		//ref_value = ptr_field;
        std::string temp_str(ptr_field, binary_len);

        ref_value = temp_str;

		return true;
	}

    bool ExtractBLOB(std::string& ref_value)
    {
        if ( (NULL == this->mysql_rows_) || (NULL == this->lengths_) || (this->index_ >= this->num_fields_) )
        {
            return false;
        }

        uint32_t cur_index = this->index_;

        const char* ptr_field = this->mysql_rows_[this->index_++];
        if (NULL == ptr_field)
        {
            return false;
        }

        unsigned long binary_len = lengths_[cur_index];
        std::string temp_str(ptr_field, binary_len);
        
        ref_value = temp_str;

        //std::cout << "ExtractBinaryString : " << binary_len << std::endl;
        return true;

    }

private:
	template <typename T>
	bool Field(T& ref_value)
	{
		if ( (NULL == this->mysql_rows_) || (this->index_ >= this->num_fields_) )
		{
			return false;
		}

        //uint32_t cur_index = this->index_;

		const char* ptr_field = this->mysql_rows_[this->index_++];
		if (NULL == ptr_field)
		{
			return false;
		}

        //unsigned long binary_len = lengths_[cur_index];

		std::stringstream strbuf;
		strbuf << ptr_field << std::endl;
		strbuf.flush();

		T new_value;
		strbuf >> new_value;

		ref_value = new_value;

		return true;
	};

	bool FieldUint8(uint8_t& ref_value)
	{
		if ((NULL == this->mysql_rows_) || (this->index_ >= this->num_fields_))
		{
			return false;
		}

		//uint32_t cur_index = this->index_;

		const char* ptr_field = this->mysql_rows_[this->index_++];
		if (NULL == ptr_field)
		{
			return false;
		}

		//unsigned long binary_len = lengths_[cur_index];

		std::stringstream strbuf;
		strbuf << ptr_field << std::endl;
		strbuf.flush();

		uint16_t new_value;
		strbuf >> new_value;

		ref_value = static_cast<uint8_t>(new_value);
		return true;
	};

	bool FieldInt8(int8_t& ref_value)
	{
		if ((NULL == this->mysql_rows_) || (this->index_ >= this->num_fields_))
		{
			return false;
		}

		//uint32_t cur_index = this->index_;

		const char* ptr_field = this->mysql_rows_[this->index_++];
		if (NULL == ptr_field)
		{
			return false;
		}

		//unsigned long binary_len = lengths_[cur_index];

		std::stringstream strbuf;
		strbuf << ptr_field << std::endl;
		strbuf.flush();

		int16_t new_value;
		strbuf >> new_value;

		ref_value = static_cast<int8_t>(new_value);
		return true;
	};

protected:
	uint32_t   index_;
	uint32_t   num_fields_;
	MYSQL_ROW  mysql_rows_;
	MYSQL_RES* ptr_mysql_res_;
    unsigned long *lengths_;
};

