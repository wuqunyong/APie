#include "string_util.h"

#include <stdint.h>
#include <algorithm>
#include <functional>

#include <openssl/sha.h>
#include <openssl/md5.h>


using namespace std;

namespace APie
{

    vector<string> StringUtil::split(const string& src, const string& delim, size_t maxParts)
    {
        if(maxParts == 0)
        {
            maxParts = size_t(-1);    
        }
        size_t lastPos = 0;
        size_t pos = 0;
        size_t size = src.size();

        vector<string> tokens;

        while(pos < size)
        {
            pos = lastPos;
            while(pos < size && delim.find_first_of(src[pos]) == string::npos)
            {
                ++pos;    
            }    

            if(pos - lastPos > 0)
            {
                if(tokens.size() == maxParts - 1)
                {
                    tokens.push_back(src.substr(lastPos));    
                    break;
                }
                else
                {
                    tokens.push_back(src.substr(lastPos, pos - lastPos));    
                }
            }

            lastPos = pos + 1;
        }

        return tokens;
    }
   
    uint32_t StringUtil::hash(const string& str)
    {
        //use elf hash
        uint32_t h = 0; 
        uint32_t x = 0;
        uint32_t i = 0;
        uint32_t len = (uint32_t)str.size();
        for(i = 0; i < len; ++i)
        {
            h = (h << 4) + str[i];
            if((x = h & 0xF0000000L) != 0)
            {
                h ^= (x >> 24);    
                h &= ~x;
            }    
        } 

        return (h & 0x7FFFFFFF);
    }
   
    string StringUtil::lower(const string& src)
    {
        string dest(src);
        
        transform(dest.begin(), dest.end(), dest.begin(), ::tolower);
        return dest;    
    }

    string StringUtil::upper(const string& src)
    {
        string dest(src);
        
        transform(dest.begin(), dest.end(), dest.begin(), ::toupper);
        return dest;    
    }

    string StringUtil::hex(const uint8_t* src, size_t srcLen)
    {
		size_t iLen = srcLen * 2 + 1;
		char *mdString = new char[iLen];
		memset(mdString, 0, iLen);

		for (size_t i = 0; i < srcLen; i++)
		{
			sprintf(&mdString[i * 2], "%02x", (unsigned int)src[i]);
		}

		std::string hexStr(mdString);
		delete[]mdString;

        return hexStr;
    }
   
    string StringUtil::hex(const string& src)
    {
        return hex((const uint8_t*)src.data(), src.size());     
    }

    string StringUtil::sha1Hex(const string& src)
    {
		uint8_t output[20] = {'\0'};
        //sha1((const uint8_t*)src.data(), src.size(), output);
		size_t iLen = src.length();
		SHA1((unsigned char*)src.c_str(), iLen, (unsigned char*)&output);

        return hex(output, 20);
    }

	string StringUtil::openSSLMD5(const string& src)
	{
		unsigned char digest[MD5_DIGEST_LENGTH];
		MD5((unsigned char*)src.c_str(), src.length(), (unsigned char*)&digest);

		return hex(digest, 16);
	}
}
