//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// string_utils:
//   String helper functions.
//

#include "string_utils.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>

namespace APie
{

const char kWhitespaceASCII[] = " \f\n\r\t\v";

std::vector<std::string> SplitString(const std::string &input,
                                     const std::string &delimiters,
                                     WhitespaceHandling whitespace,
                                     SplitResult resultType)
{
    std::vector<std::string> result;
    if (input.empty())
    {
        return result;
    }

    std::string::size_type start = 0;
    while (start != std::string::npos)
    {
        std::string::size_type end = input.find_first_of(delimiters, start);

        std::string piece;
        if (end == std::string::npos)
        {
            piece = input.substr(start);
            start = std::string::npos;
        }
        else
        {
            piece = input.substr(start, end - start);
            start = end + 1;
        }

        if (whitespace == TRIM_WHITESPACE)
        {
            piece = TrimString(piece, kWhitespaceASCII);
        }

        if (resultType == SPLIT_WANT_ALL || !piece.empty())
        {
            result.push_back(piece);
        }
    }

    return result;
}

void SplitStringAlongWhitespace(const std::string &input,
                                std::vector<std::string> *tokensOut)
{

    std::istringstream stream(input);
    std::string line;

    while (std::getline(stream, line))
    {
        size_t prev = 0, pos;
        while ((pos = line.find_first_of(kWhitespaceASCII, prev)) != std::string::npos)
        {
            if (pos > prev)
                tokensOut->push_back(line.substr(prev, pos - prev));
            prev = pos + 1;
        }
        if (prev < line.length())
            tokensOut->push_back(line.substr(prev, std::string::npos));
    }
}

std::string TrimString(const std::string &input, const std::string &trimChars)
{
    std::string::size_type begin = input.find_first_not_of(trimChars);
    if (begin == std::string::npos)
    {
        return "";
    }

    std::string::size_type end = input.find_last_not_of(trimChars);
    if (end == std::string::npos)
    {
        return input.substr(begin);
    }

    return input.substr(begin, end - begin + 1);
}

std::string StringJoin(std::vector<std::string> &data, const std::string &delimiters)
{
	std::string output;
	for (std::vector<std::string>::iterator ite = data.begin();
		ite != data.end();
		/*++ite*/)
	{
		output += *ite;
		++ite;
		if (ite != data.end())
		{
			output += delimiters;
		}
	}

	return output;
}

bool HexStringToUInt(const std::string &input, unsigned int *uintOut)
{
    unsigned int offset = 0;

    if (input.size() >= 2 && input[0] == '0' && input[1] == 'x')
    {
        offset = 2u;
    }

    // Simple validity check
    if (input.find_first_not_of("0123456789ABCDEFabcdef", offset) != std::string::npos)
    {
        return false;
    }

    std::stringstream inStream(input);
    inStream >> std::hex >> *uintOut;
    return !inStream.fail();
}

bool ReadFileToString(const std::string &path, std::string *stringOut)
{
    std::ifstream inFile(path.c_str());
    if (inFile.fail())
    {
        return false;
    }

    inFile.seekg(0, std::ios::end);
    stringOut->reserve(static_cast<std::string::size_type>(inFile.tellg()));
    inFile.seekg(0, std::ios::beg);

    stringOut->assign(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>());
    return !inFile.fail();
}

bool BeginsWith(const std::string &str, const std::string &prefix)
{
    return strncmp(str.c_str(), prefix.c_str(), prefix.length()) == 0;
}

bool BeginsWith(const std::string &str, const char *prefix)
{
    return strncmp(str.c_str(), prefix, strlen(prefix)) == 0;
}

bool BeginsWith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool BeginsWith(const std::string &str, const std::string &prefix, const size_t prefixLength)
{
    return strncmp(str.c_str(), prefix.c_str(), prefixLength) == 0;
}

bool EndsWith(const std::string &str, const char *suffix)
{
    size_t len = strlen(suffix);
    if (len > str.size())
        return false;

    const char *end = str.c_str() + str.size() - len;

    return memcmp(end, suffix, len) == 0;
}

void ToLower(std::string *str)
{
    for (std::string::iterator it = (*str).begin(); it != (*str).end(); ++it)
    {
        *it = static_cast<char>(::tolower(*it));
    }    
}

bool ReplaceSubstring(std::string *str,
                      const std::string &substring,
                      const std::string &replacement)
{
    size_t replacePos = str->find(substring);
    if (replacePos == std::string::npos)
    {
        return false;
    }
    str->replace(replacePos, substring.size(), replacement);
    return true;
}

std::string& ReplaceStrAll(std::string& str, const std::string& old_value, const std::string& new_value)
{
	while (true)
	{
		std::string::size_type pos(0);
		if ((pos = str.find(old_value)) != std::string::npos)
		{
			str.replace(pos, old_value.length(), new_value);
		}
		else
		{
			break;
		}
	}
	return   str;
}

static int tohex(int c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else
		return (-1);
}

char* URLDecode(const char *in)
{
	char *out, *d;
	const char *s;

	out = (char *)malloc(strlen(in) + 1);
	if (out == NULL)
		return (NULL);
	for (s = in, d = out; *s != '\0'; ) {
		if (s[0] == '%' && s[1] != '\0' && s[2] != '\0') {
			/* Try to convert % escape */
			int digit1 = tohex(s[1]);
			int digit2 = tohex(s[2]);
			if (digit1 >= 0 && digit2 >= 0) {
				/* Looks good, consume three chars */
				s += 3;
				/* Convert output */
				*d++ = ((digit1 << 4) | digit2);
				continue;
			}
			/* Else fall through and treat '%' as normal char */
		}
		*d++ = *s++;
	}
	*d = '\0';
	return (out);
}


std::string randomStr(int32_t iCount)
{
	std::string output;

	constexpr char CHAR_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

	int iLen = strlen(CHAR_TABLE);
	while (iCount > 0)
	{
		iCount--;
		int index = rand() % iLen;
		output.push_back(CHAR_TABLE[index]);
	}

	return output;
}


}  // namespace APie
