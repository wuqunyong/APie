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

/**
 * @file
 */

#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>


namespace APie {
namespace common {

/**
 * @brief Get file content as string.
 * @param file_name The name of the file to read content.
 * @param content The file content.
 * @return If the action is successful.
 */
bool GetContent(const std::string &file_name, std::string *content);

/**
 * @brief Get absolute path by concatenating prefix and relative_path.
 * @return The absolute path.
 */
std::string GetAbsolutePath(const std::string &prefix,
                            const std::string &relative_path);

/**
 * @brief Check if the path exists.
 * @param path a file name, such as /a/b/c.txt
 * @return If the path exists.
 */
bool PathExists(const std::string &path);

/**
 * @brief Check if the directory specified by directory_path exists
 *        and is indeed a directory.
 * @param directory_path Directory path.
 * @return If the directory specified by directory_path exists
 *         and is indeed a directory.
 */
bool DirectoryExists(const std::string &directory_path);

/**
 * @brief Copy a file.
 * @param from The file path to copy from.
 * @param to The file path to copy to.
 * @return If the action is successful.
 */
bool CopyFile(const std::string &from, const std::string &to);

std::string GetFileName(const std::string &path);

int64_t FileSize(const std::string& path);

}  // namespace common
}  // namespace APie
