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

#include "../common/file.h"
#include "../network/logger.h"

#include <errno.h>
#include <stddef.h>
#include <fstream>
#include <string>
#include <sstream>

namespace APie {
namespace Common {

bool GetContent(const std::string &file_name, std::string *content) {
  std::ifstream fin(file_name);
  if (!fin) {
    return false;
  }

  std::stringstream str_stream;
  str_stream << fin.rdbuf();
  *content = str_stream.str();
  return true;
}

std::string GetAbsolutePath(const std::string &prefix,
                            const std::string &relative_path) {
  if (relative_path.empty()) {
    return prefix;
  }
  // If prefix is empty or relative_path is already absolute.
  if (prefix.empty() || relative_path.front() == '/') {
    return relative_path;
  }

  if (prefix.back() == '/') {
    return prefix + relative_path;
  }
  return prefix + "/" + relative_path;
}

bool PathExists(const std::string &path) {
  struct stat info;
  return stat(path.c_str(), &info) == 0;
}

bool DirectoryExists(const std::string &directory_path) {
  struct stat info;
  if (stat(directory_path.c_str(), &info) != 0) {
    return false;
  }

  if (info.st_mode & S_IFDIR) {
    return true;
  }

  return false;
}

bool CopyFile(const std::string &from, const std::string &to) {
  std::ifstream src(from, std::ios::binary);
  if (!src) {
	std::stringstream ss;
	ss << "Source path doesn't exist: " << from;
	ASYNC_PIE_LOG("file:%s", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
    return false;
  }

  std::ofstream dst(to, std::ios::binary);
  if (!dst) {
	std::stringstream ss;
	ss << "Target path is not writable: " << to;
	ASYNC_PIE_LOG("file:%s", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
    return false;
  }

  dst << src.rdbuf();
  return true;
}

std::string GetFileName(const std::string &path) {
  std::string filename;
  std::string::size_type loc = path.rfind('/');
  if (loc == std::string::npos) {
    filename = path;
  } else {
    filename = path.substr(loc + 1);
  }
  return filename;
}

int64_t FileSize(const std::string& path) {
#ifdef WIN32
	struct _stat info;
	if (::_stat(path.c_str(), &info) != 0) {
		return -1;
	}
	return info.st_size;
#else
	struct stat info;
	if (::stat(path.c_str(), &info) != 0) {
		return -1;
	}
	return info.st_size;
#endif
}

int64_t FileDataModificationTime(const std::string& path)
{
#ifdef WIN32
	struct _stat info;
	if (::_stat(path.c_str(), &info) != 0) {
		return -1;
	}
	return info.st_mtime;
#else
	struct stat info;
	if (::stat(path.c_str(), &info) != 0) {
		return -1;
	}
	return info.st_mtime;
#endif
}

}  // namespace common
}  // namespace APie
