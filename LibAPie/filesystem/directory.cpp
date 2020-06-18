#include "directory.h"

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif
#include <assert.h>

#ifdef WIN32
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#else
#define ACCESS access
#define MKDIR(a) mkdir((a),0755)
#endif

namespace APie {
namespace Filesystem {

std::string Directory::sCurPath;

bool Directory::createDirectory(const char* filepath)
{
	assert(filepath);
	size_t iLen = strlen(filepath);
	for (size_t i = 0; i < iLen; i++)
	{
		char tempChar = filepath[i];
		if (tempChar == '/' || tempChar == '\\' || tempChar == '\0' || i == iLen - 1)
		{
			std::string sCurPath;
			if (i == iLen - 1)
			{
				std::string sPath(filepath, 0, i + 1);
				sCurPath = sPath;
			}
			else
			{
				if (i == 0)
				{
					std::string sPath("/");
					sCurPath = sPath;
				}
				else
				{
					std::string sPath(filepath, 0, i);
					sCurPath = sPath;
				}

			}
			const char* ptrPath = sCurPath.c_str();
			if (!Directory::isExist(ptrPath))
			{
				if (MKDIR(ptrPath) == 0)
				{
					//return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				//return true;
			}
		}

	}

	return true;

}

bool Directory::isExist(const char* filepath)
{
	assert(filepath);
	if ((ACCESS(filepath, 0)) == 0)
	{
		return true;
	}
	return false;
}

std::string Directory::getCWD()
{
#ifdef WIN32
	char cwd[MAX_PATH]; // not thread safe!
	return ::_getcwd(cwd, MAX_PATH);
#else  // LINUX, OSX
	char cwd[PATH_MAX];
	return ::getcwd(cwd, PATH_MAX);
#endif
}

void Directory::initCurPath()
{
	std::string sPath = Directory::getCWD();
	Directory::sCurPath = sPath;
}

const char * Directory::basename(const char *path)
{
	const char *basename;

	basename = strrchr(path, '/');
	return (basename ? (basename + 1) : path);
}

int Directory::isdir(char const * path)
{
#ifdef WIN32
	return false;
#else
	struct stat64 my_stat;

	if (-1 == lstat64(path, &my_stat))
	{
		if (errno == ENOENT)
		{
			return 0;
		}

		fprintf(stderr, "Stat failed on %s: %s\n", path, strerror(errno));
		return 0;
	}

	return S_ISDIR(my_stat.st_mode) && !S_ISLNK(my_stat.st_mode);
#endif
}

}
}