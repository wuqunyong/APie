#pragma once

#include <string>
#include "../network/windows_platform.h"


namespace APie {
namespace Filesystem {


class Directory {
public:
	static bool createDirectory(const char* filepath);
	static bool isExist(const char* filepath);
	static std::string getCWD();
	static void initCurPath();
	static const char * basename(const char *path);
	static int isdir(char const * path);

public:
	static std::string sCurPath;
};

} // namespace Filesystem
} // namespace APie
