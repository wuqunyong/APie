#include "byte_buffer.h"

#include <assert.h>
#include <string>
#include <exception>
#include <cstring>
#include <malloc.h>
#include <new>

ByteBuffer::ByteBuffer(const unsigned char* const data, size_t size)
{
	this->buf_size_ = size;

	if (this->buf_size_ > 0)
	{
		this->buf_ = const_cast<unsigned char*> (data);
	}
	else
	{
		this->buf_ = NULL;
	}

	this->rd_pos_ = 0;
	this->wr_pos_ = 0;

	this->Length(this->buf_size_);
}

ByteBuffer::ByteBuffer(size_t buf_size)
{
	this->buf_size_ = buf_size;

	if (this->buf_size_ > 0)
	{
		this->buf_ = (unsigned char *) malloc(this->buf_size_);

		if (this->buf_ == NULL)
		{
			throw std::bad_alloc();
		}
	}
	else
	{
		this->buf_ = NULL;
	}

	this->rd_pos_ = 0;
	this->wr_pos_ = 0;
}

ByteBuffer::~ByteBuffer()
{
	Kill();
}

void ByteBuffer::Kill(void)
{
	if (this->buf_)
	{
		free(this->buf_);
		this->buf_ = NULL;
	}
}

unsigned char* ByteBuffer::Buf(void)
{
	return this->buf_;
}

unsigned char* ByteBuffer::RdPos(void)
{
	return this->buf_ + this->rd_pos_;
}

void ByteBuffer::AddRdPos(size_t bytes)
{
	this->Skip(bytes);
}

const unsigned char* ByteBuffer::Buf(void) const
{
	return this->buf_;
}

void ByteBuffer::Length(size_t length)
{
	this->wr_pos_ = this->rd_pos_ + length;
}

size_t ByteBuffer::Length(void)
{
	return this->wr_pos_ - this->rd_pos_;
}

size_t ByteBuffer::Space(void)
{
	return this->buf_size_ - this->wr_pos_;
}

size_t ByteBuffer::Size(void)
{
	return this->buf_size_;
}

void ByteBuffer::Align(void)
{
	size_t data_len = this->wr_pos_ - this->rd_pos_;
	memmove(this->buf_, this->buf_ + this->rd_pos_, data_len);
	this->rd_pos_ = 0;
	this->wr_pos_ = data_len;
}

unsigned char* ByteBuffer::Grow(size_t bytes)
{
	size_t need = this->wr_pos_ + bytes;
	if (this->buf_size_ < need)
	{
		this->Expand(bytes);
	}

	size_t old_wr_pos = this->wr_pos_;
	this->wr_pos_ += bytes;
	return this->buf_ + old_wr_pos;
}

//从当前有效缓冲区的内存起始地址删除指定长度bytes字节数据
char* ByteBuffer::Drain(size_t bytes)
{
	if (this->wr_pos_ <= this->rd_pos_)
	{
		return NULL;
	}

	size_t data_len = this->wr_pos_ - this->rd_pos_;
	if (bytes > data_len)
	{
		return NULL;
	}

	//char *temp_buff = new char[bytes];
	char *temp_buff =(char *) malloc(bytes);
	memcpy(temp_buff, this->buf_ + this->rd_pos_, bytes);
	this->rd_pos_ += bytes;

	if (this->rd_pos_ == this->wr_pos_)
	{
		this->rd_pos_ = 0;
		this->wr_pos_ = 0;

	}
	return temp_buff;
}

void ByteBuffer::Expand(size_t bytes)
{
	size_t need = this->wr_pos_ + bytes;
	if (this->buf_size_ >= need)
	{
		return;
	}

	if (this->rd_pos_ >= bytes)
	{
		this->Align();
	}
	else
	{
		size_t realloc_size = this->buf_size_ << 1;//* 2;
		if (realloc_size < 512)
		{
			realloc_size = 512;
		}
		if (need > realloc_size)
		{
			realloc_size = need + (1 << 14);//16 * 1024;
		}

		if (realloc_size > (1 << 28) /*64 * 1024 * 1024 * 4*/)
		{
			throw std::exception();
		}

		if (this->rd_pos_ != 0)
		{
			this->Align();
		}

		this->buf_ = (unsigned char *) realloc(this->buf_, realloc_size);
		this->buf_size_ = realloc_size;
	}

}


ByteBuffer& ByteBuffer::operator << (int8_t value)
{
    return this->Append<int8_t> (value);
}

ByteBuffer& ByteBuffer::operator << (int16_t value)
{
    return this->Append<int16_t> (value);
}

ByteBuffer& ByteBuffer::operator << (int32_t value)
{
    return this->Append<int32_t> (value);
}

ByteBuffer& ByteBuffer::operator << (int64_t value)
{
    return this->Append<int64_t> (value);
}

ByteBuffer& ByteBuffer::operator << (uint8_t value)
{
    return this->Append<uint8_t> (value);
}

ByteBuffer& ByteBuffer::operator << (uint16_t value)
{
    return this->Append<uint16_t> (value);
}

ByteBuffer& ByteBuffer::operator << (uint32_t value)
{
    return this->Append<uint32_t> (value);
}

ByteBuffer& ByteBuffer::operator << (uint64_t value)
{
    return this->Append<uint64_t> (value);
}

ByteBuffer& ByteBuffer::operator << (float value)
{
    return this->Append<float> (value);
}

ByteBuffer& ByteBuffer::operator << (double value)
{
    return this->Append<double> (value);
}

ByteBuffer& ByteBuffer::operator << (bool value)
{
    return this->Append<int8_t> (value ? 1 : 0);
}


ByteBuffer& ByteBuffer::operator >> (int8_t& value)
{
    return this->Extract<int8_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (int16_t& value)
{
    return this->Extract<int16_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (int32_t& value)
{
    return this->Extract<int32_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (int64_t& value)
{
    return this->Extract<int64_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (uint8_t& value)
{
    return this->Extract<uint8_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (uint16_t& value)
{
    return this->Extract<uint16_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (uint32_t& value)
{
    return this->Extract<uint32_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (uint64_t& value)
{
    return this->Extract<uint64_t> (value);
}

ByteBuffer& ByteBuffer::operator >> (float& value)
{
    return this->Extract<float> (value);
}

ByteBuffer& ByteBuffer::operator >> (double& value)
{
    return this->Extract<double> (value);
}

ByteBuffer& ByteBuffer::operator >> (bool& value)
{
    int8_t temp_value;
    this->Extract<int8_t> (temp_value);

    value = (temp_value == 0) ? false : true;

	return *this;
}

void ByteBuffer::Skip(size_t bytes)
{
    this->rd_pos_ += bytes;

    if (this->rd_pos_ > this->wr_pos_)
    {
		throw std::exception();
    }
}

unsigned char* ByteBuffer::Read(size_t bytes)
{
    size_t old_rd_pos = this->rd_pos_;
    this->Skip(bytes);
    return this->buf_ + old_rd_pos;
}
