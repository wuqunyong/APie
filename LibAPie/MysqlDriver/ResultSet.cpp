#include "ResultSet.h"

ResultSet::ResultSet(MYSQL_RES* ptr_mysql_res) : ptr_mysql_res_(ptr_mysql_res)
{
	this->num_fields_ = mysql_num_fields(this->ptr_mysql_res_);
    lengths_ = NULL;
}

ResultSet::~ResultSet(void)
{
	mysql_free_result(this->ptr_mysql_res_);
}

bool ResultSet::MoveNext()
{
	this->index_ = 0x00;
	this->mysql_rows_ = mysql_fetch_row(this->ptr_mysql_res_);
    if (this->mysql_rows_ != NULL)
    {
        lengths_ = mysql_fetch_lengths(this->ptr_mysql_res_);
    }
    else
    {
        lengths_ = NULL;
    }
    
	return (this->mysql_rows_ != NULL);
}


bool ResultSet::operator>> (int8_t& ref_value)
{
	return this->Field<int8_t>(ref_value);
}

bool ResultSet::operator>> (int16_t& ref_value)
{
	return this->Field<int16_t>(ref_value);
}

bool ResultSet::operator>> (int32_t& ref_value)
{
	return this->Field<int32_t>(ref_value);
}

bool ResultSet::operator>> (int64_t& ref_value)
{
	return this->Field<int64_t>(ref_value);
}

bool ResultSet::operator>> (uint8_t& ref_value)
{
	return this->Field<uint8_t>(ref_value);
}

bool ResultSet::operator>> (uint16_t& ref_value)
{
	return this->Field<uint16_t>(ref_value);
}

bool ResultSet::operator>> (uint32_t& ref_value)
{
	return this->Field<uint32_t>(ref_value);
}

bool ResultSet::operator>> (uint64_t& ref_value)
{
	return this->Field<uint64_t>(ref_value);
}
