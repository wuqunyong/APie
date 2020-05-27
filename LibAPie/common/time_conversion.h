/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <stdint.h>
#include <ctime>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace APie {

	inline uint64_t StringToUnixSeconds(const std::string& time_str)
	{
		const std::string& format_str = "%d-%d-%d %d:%d:%d";

		struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
		//strptime(time_str.c_str(), format_str.c_str(), tmp_time);
		int iResult = sscanf(time_str.c_str(), format_str.c_str(),
			&tmp_time->tm_year, &tmp_time->tm_mon, &tmp_time->tm_mday,
			&tmp_time->tm_hour, &tmp_time->tm_min, &tmp_time->tm_sec);
		if (iResult != 6)
		{
			return 0;
		}

		tmp_time->tm_year -= 1900;
		tmp_time->tm_mon--;
		time_t time = mktime(tmp_time);
		free(tmp_time);
		return (uint64_t)time;
	}

	inline std::string UnixSecondsToString(uint64_t unix_seconds, const std::string& format_str = "%Y-%m-%d %H:%M:%S") 
	{
		std::time_t t = unix_seconds;
		//struct tm* pt = std::gmtime(&t);
		struct tm* pt = std::localtime(&t);
		if (pt == nullptr) 
		{
			return std::string("");
		}
		uint32_t length = 64;
		std::vector<char> buff(length, '\0');
		strftime(buff.data(), length, format_str.c_str(), pt);
		return std::string(buff.data());
	}

}


