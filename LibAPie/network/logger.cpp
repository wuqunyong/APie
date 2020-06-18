#include "logger.h"

#include <time.h>
#include <map>
#include <sstream>
#include <mutex>

#ifdef WIN32

#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

#include "Ctx.h"
#include "Command.h"

#include "../filesystem/directory.h"
#include "../common/string_utils.h"


static std::mutex log_sync;

std::map<std::string, LogFile*> cacheMap;

std::string getLogLevelName(int level)
{
	std::string sLevelName;
	switch(level)
	{
	case PIE_DEBUG:
		{
			sLevelName = "PIE_DEBUG";
			break;
		}
	case PIE_VERBOSE:
		{
			sLevelName = "PIE_VERBOSE";
			break;
		}
	case PIE_NOTICE:
		{
			sLevelName = "PIE_INFO";
			break;
		}
	case PIE_WARNING:
		{
			sLevelName = "PIE_WARNING";
			break;
		}
	case PIE_ERROR:
		{
			sLevelName = "PIE_ERROR";
			break;
		}
	case PIE_RECORD:
		{
			sLevelName = "PIE_RECORD";
			break;
		}
	default:
		{
			char temp[64] = {'\0'};
			sprintf(temp,"Level:%d",level);
			sLevelName = temp;
		}
	}

	return sLevelName;
}

void pieLogRaw(const char* file, int cycle, int level, const char* msg)
{
	// 多线程，同时访问所以要加锁
	assert(file != NULL);

	std::lock_guard<std::mutex> guard(log_sync);

	bool bMergeFlag = false;

	LogFile* ptrFile = NULL;
	//bool bMerge = APie::Ctx::getConfigReader()->GetBoolean("log", "merge", true);
	if (true)
	{
		bMergeFlag = true;
		ptrFile = getCacheFile("poseidon_mgr.log", PIE_CYCLE_DAY);
	}
	else
	{
		ptrFile = getCacheFile(file, cycle);
	}
		
	if (!ptrFile)
	{
		printf("getCacheFile %s error!",file);
		return;
	}
	
	char timebuf[64]={'\0'};
	time_t now = time(NULL);
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",localtime(&now));

	struct timeval tp;
	evutil_gettimeofday(&tp, NULL);
	uint64_t iMilliSecond = ((tp.tv_sec * (uint64_t) 1000000 + tp.tv_usec) / 1000);

	std::string sLevelName = getLogLevelName(level);
	if (bMergeFlag)
	{
		fprintf(ptrFile->pFile, "%s|%llu|%s|TAG:%s|%s\n", timebuf, (long long unsigned int)iMilliSecond, sLevelName.c_str(), file, msg);
	} 
	else
	{
		fprintf(ptrFile->pFile, "%s|%llu|%s|%s\n", timebuf, (long long unsigned int)iMilliSecond, sLevelName.c_str(), msg);
	}
	fflush(ptrFile->pFile);
}

//file="directory/filename"
void pieLog(const char* file, int cycle, int level, const char *fmt, ...)
{
	va_list ap;
	char msg[PIE_MAX_LOGMSG_LEN];

	//TODO
	int iConfigLogLevel = 1;

	if ((level&0xff) < iConfigLogLevel)
	{
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	pieLogRaw(file,cycle,level,msg);
}

void asyncPieLog(const char* file, int cycle, int level, const char *fmt, ...)
{
	va_list ap;
	char msg[PIE_MAX_LOGMSG_LEN] = {'\0'};

	//TODO
	int iConfigLogLevel = 1;
	if ((level&0xff) < iConfigLogLevel)
	{
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	if (NULL == APie::CtxSingleton::get().getLogThread())
	{
		pieLogRaw(file, cycle, level, msg);
		return;
	}

	APie::LogCmd* ptrCmd = new APie::LogCmd;
	ptrCmd->sFile = file;
	ptrCmd->iCycle = cycle;
	ptrCmd->iLevel = level;
	ptrCmd->sMsg = msg;

	APie::Command cmd;
	cmd.type = APie::Command::async_log;
	cmd.args.async_log.ptrData = ptrCmd;
	APie::CtxSingleton::get().getLogThread()->push(cmd);
}


LogFile* openFile(std::string file, int cycle)
{
	LogFile* ptrFile = new LogFile;

	std::string sFile;

	std::string sCurPath = APie::Filesystem::Directory::getCWD();
	
	//TODO
	std::string sLogPrefix = "app";

#ifdef WIN32
	sFile = sCurPath;
	sFile.append("/Log/");
	sFile.append(sLogPrefix);
	sFile.append("/");
	sFile.append(file);
#else
	sFile = "/usr/local/poseidon-mgr/logs/";
	//bool bMerge = APie::Ctx::getConfigReader()->GetBoolean("log", "merge", true);
	if (true)
	{
		//nothing
	}
	else
	{
		sFile.append("agent/");
	}
	sFile.append(file);
#endif
	
	APie::ReplaceStrAll(sFile,"\\","/");

	std::string::size_type pos = sFile.rfind ("/");
	if (pos == std::string::npos) 
	{
		delete ptrFile;
		return NULL;
	}
	std::string sPath = sFile.substr(0, pos);
	std::string sFileName = sFile.substr(pos + 1);

	APie::Filesystem::Directory::createDirectory(sPath.c_str());
	time_t now = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&now);

	//char file_postfix[64]={'\0'};
	//strftime(file_postfix, sizeof(file_postfix), "%Y%m%d%H-%M%S",localtime(&now));

	std::string sNewFileName;
	sNewFileName = sFileName;
	//sNewFileName.append(".");
	//sNewFileName.append(file_postfix);

	std::string sNewName;
	sNewName = sPath;
	sNewName.append("/");
	sNewName.append(sNewFileName);

	FILE * pFile = fopen(sNewName.c_str(),"a");
	if (!pFile)
	{
		delete ptrFile;
		return NULL;
	}

	ptrFile->pFile = pFile;
	ptrFile->sFile = sNewName;
	ptrFile->iCycle = cycle;
	ptrFile->iCreateYear = timeinfo->tm_year;
	ptrFile->iCreateMonth = timeinfo->tm_mon;
	ptrFile->iCreateDay = timeinfo->tm_mday;
	ptrFile->iCreateHour = timeinfo->tm_hour;
	ptrFile->iCreateMinute = timeinfo->tm_min;

	return ptrFile;
}

void closeFile(LogFile* ptrFile)
{
	fflush(ptrFile->pFile);
	fclose(ptrFile->pFile);
}

void moveFile(std::string from, std::string to)
{
	time_t now = time(NULL);
	char curDate[64] = { '\0' };
	strftime(curDate, sizeof(curDate), "%Y-%m-%d", localtime(&now));
	to.append(curDate);
	APie::Filesystem::Directory::createDirectory(to.c_str());

	std::string baseName = APie::Filesystem::Directory::basename(from.c_str());
	char filePostfix[64] = { '\0' };
	strftime(filePostfix, sizeof(filePostfix), "%Y%m%d-%H%M", localtime(&now));
	std::string sNewFileName;
	sNewFileName = baseName;
	sNewFileName.append(".");
	sNewFileName.append(filePostfix);


	std::string target = to + "/" + sNewFileName;
	int result = rename(from.c_str(), target.c_str());
	if (result == 0)
	{
		asyncPieLog("move/log", PIE_CYCLE_DAY, PIE_DEBUG, "rename successfully|%s  ->  %s",
			from.c_str(), target.c_str());
	}
	else
	{
		asyncPieLog("move/log", PIE_CYCLE_DAY, PIE_ERROR, "rename error|%s  ->  %s",
			from.c_str(), target.c_str());
	}
}

void checkRotate()
{
	std::lock_guard<std::mutex> guard(log_sync);

	std::map<std::string, LogFile*>::iterator ite = cacheMap.begin();
	while (ite != cacheMap.end())
	{
		if (isChangeFile(ite->second, ite->second->iCycle))
		{
			std::string fromFile = ite->second->sFile;
			closeFile(ite->second);
			delete ite->second;

			std::map<std::string, LogFile*>::iterator o = ite;
			++ite;
			cacheMap.erase(o);

			std::string toDir = "/usr/local/poseidon-mgr/logs-backup/";
			moveFile(fromFile, toDir);
			continue;
		}

		++ite;
	}
}

void logFileClose()
{
	std::map<std::string, LogFile*>::iterator ite = cacheMap.begin();
	while (ite != cacheMap.end())
	{
		closeFile(ite->second);
		delete ite->second;

		std::map<std::string, LogFile*>::iterator o = ite;
		++ite;
		cacheMap.erase(o);
	}
}

LogFile* getCacheFile(std::string file, int cycle)
{
	std::map<std::string, LogFile*>::iterator ite = cacheMap.find(file);
	if (ite != cacheMap.end())
	{
		return ite->second;
	}

	LogFile* ptrFile = openFile(file, cycle);
	if (!ptrFile)
	{
		return NULL;
	}
	
	cacheMap[file] = ptrFile;
	return ptrFile;
}

bool isChangeFile(LogFile* ptrFile, int cycle)
{
#ifdef WIN32
	int fileFd = _fileno(ptrFile->pFile);
#else
	int fileFd = fileno(ptrFile->pFile);
#endif
	struct stat statInfo;
	if (0 == fstat(fileFd, &statInfo))
	{
		//TODO
		int32_t iSize = 128;
		if (statInfo.st_size > 1024 * 1024 * iSize)
		{
			return true;
		}
	}

	time_t now = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&now);

	if (ptrFile->iCreateYear != timeinfo->tm_year || ptrFile->iCreateMonth != timeinfo->tm_mon)
	{
		return true;
	}

	switch (cycle)
	{
	case PIE_CYCLE_DAY:
		if (ptrFile->iCreateDay != timeinfo->tm_mday)
		{
			return true;
		}
		break;
	case PIE_CYCLE_HOUR:
		if (ptrFile->iCreateDay != timeinfo->tm_mday || ptrFile->iCreateHour != timeinfo->tm_hour)
		{
			return true;
		}
		break;
	case PIE_CYCLE_MINUTE:
		if (ptrFile->iCreateDay != timeinfo->tm_mday || ptrFile->iCreateHour != timeinfo->tm_hour || ptrFile->iCreateMinute != timeinfo->tm_min)
		{
			return true;
		}
		break;
	}
	
	return false;
}

void fatalExit(const char* message)
{
	fprintf(stderr, "%s: %s\n", "fatalExit", message);
	pieLog("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s\n", "fatalExit", message);

	exit(EXIT_FAILURE);
}