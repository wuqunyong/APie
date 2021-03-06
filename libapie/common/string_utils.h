//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// string_utils:
//   String helper functions.
//

#pragma once

#include <string>
#include <vector>
#include <sstream>



namespace APie
{

extern const char kWhitespaceASCII[];

enum WhitespaceHandling
{
    KEEP_WHITESPACE,
    TRIM_WHITESPACE,
};

enum SplitResult
{
    SPLIT_WANT_ALL,
    SPLIT_WANT_NONEMPTY,
};

std::vector<std::string> SplitString(const std::string &input,
                                     const std::string &delimiters,
                                     WhitespaceHandling whitespace,
                                     SplitResult resultType);

void SplitStringAlongWhitespace(const std::string &input,
                                std::vector<std::string> *tokensOut);

std::string TrimString(const std::string &input, const std::string &trimChars);

std::string StringJoin(std::vector<std::string> &data, const std::string &delimiters);

bool HexStringToUInt(const std::string &input, unsigned int *uintOut);

bool ReadFileToString(const std::string &path, std::string *stringOut);


// Check if the string str begins with the given prefix.
// The comparison is case sensitive.
bool BeginsWith(const std::string &str, const std::string &prefix);

// Check if the string str begins with the given prefix.
// Prefix may not be NULL and needs to be NULL terminated.
// The comparison is case sensitive.
bool BeginsWith(const std::string &str, const char *prefix);

// Check if the string str begins with the given prefix.
// str and prefix may not be NULL and need to be NULL terminated.
// The comparison is case sensitive.
bool BeginsWith(const char *str, const char *prefix);

// Check if the string str begins with the first prefixLength characters of the given prefix.
// The length of the prefix string should be greater than or equal to prefixLength.
// The comparison is case sensitive.
bool BeginsWith(const std::string &str, const std::string &prefix, const size_t prefixLength);

// Check if the string str ends with the given suffix.
// Suffix may not be NUL and needs to be NULL terminated.
// The comparison is case sensitive.
bool EndsWith(const std::string& str, const char* suffix);

// Convert to lower-case.
void ToLower(std::string *str);

// Replaces the substring 'substring' in 'str' with 'replacement'. Returns true if successful.
bool ReplaceSubstring(std::string *str,
                      const std::string &substring,
                      const std::string &replacement);

std::string& ReplaceStrAll(std::string& str, const std::string& old_value, const std::string& new_value);

char* URLDecode(const char *in);

std::string randomStr(int32_t iCount);


template <typename S, typename T>
struct is_streamable {
	template <typename SS, typename TT>
	static auto test(int)
		-> decltype(std::declval<SS&>() << std::declval<TT>(), std::true_type());

	template <typename, typename>
	static auto test(...)->std::false_type;

	static const bool value = decltype(test<S, T>(0))::value;
};

template<typename Key, bool Streamable>
struct streamable_to_string {
	static std::string impl(const Key& key) {
		std::stringstream ss;
		ss << key;
		return ss.str();
	}
};

template<typename Key>
struct streamable_to_string<Key, false> {
	static std::string impl(const Key&) {
		return "undefined";
	}
};

template<typename Key>
std::string key_to_string(const Key& key) {
	return streamable_to_string<Key, is_streamable<std::stringstream, Key>::value>().impl(key);
}

}  // namespace APie
