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

#include "ctx.h"
#include "command.h"

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
	case PIE_PANIC:
	{
		sLevelName = "PIE_PANIC";
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

void pieLogRaw(const char* file, int cycle, int level, const char* msg, bool ignoreMerge)
{
	// 多线程，同时访问所以要加锁
	assert(file != NULL);

	std::lock_guard<std::mutex> guard(log_sync);

	bool bMergeFlag = false;

	std::string logFileName;

	LogFile* ptrFile = NULL;
	bool bMerge = APie::CtxSingleton::get().yamlAs<bool>({"log","merge"}, true); 
	if (bMerge && !ignoreMerge)
	{
		bMergeFlag = true;
		//std::string logName = APie::CtxSingleton::get().yamlAs<std::string>({ "log", "name" }, "apie");
		logFileName = APie::Ctx::logName() + "-" + APie::Ctx::logPostfix();
		ptrFile = getCacheFile(logFileName, PIE_CYCLE_DAY);
	}
	else
	{
		std::string tmpFile(file);
		logFileName = tmpFile + "-" + APie::Ctx::logPostfix();
		ptrFile = getCacheFile(logFileName, cycle);
	}
		
	if (!ptrFile)
	{
		printf("getCacheFile %s error!", logFileName.c_str());
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

	bool bShowConsole = APie::CtxSingleton::get().yamlAs<bool>({ "log","show_console" }, false);
	if (bShowConsole || level >= PIE_ERROR)
	{
#ifdef WIN32
		switch (level)
		{
		case PIE_DEBUG:
		case PIE_VERBOSE:
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		}
		case PIE_NOTICE:
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
			break;
		}
		case PIE_WARNING:
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
			break;
		}
		case PIE_ERROR:
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED);
			break;
		}
		case PIE_PANIC:
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | BACKGROUND_GREEN);
			break;
		}
		default:
			break;
		}
		printf("%s|%llu|%s|TAG:%s|%s\n", timebuf, (long long unsigned int)iMilliSecond, sLevelName.c_str(), file, msg);

		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
#else
		bool bDaemon = APie::CtxSingleton::get().isDaemon();
		if (!bDaemon)
		{
			printf("%s|%llu|%s|TAG:%s|%s\n", timebuf, (long long unsigned int)iMilliSecond, sLevelName.c_str(), file, msg);
		}
#endif
	}
}

//file="directory/filename"
void pieLog(const char* file, int cycle, int level, const char *fmt, ...)
{
	va_list ap;
	char msg[PIE_MAX_LOGMSG_LEN];

	int iConfigLogLevel = APie::CtxSingleton::get().yamlAs<int>({ "log","level" }, 2);
	if ((level&0xff) < iConfigLogLevel)
	{
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	pieLogRaw(file,cycle,level,msg,false);
}

void asyncPieLog(const char* file, int cycle, int level, const char *fmt, ...)
{
	va_list ap;
	char msg[PIE_MAX_LOGMSG_LEN] = {'\0'};

	int iConfigLogLevel = APie::CtxSingleton::get().yamlAs<int>({ "log","level" }, 2);
	if ((level&0xff) < iConfigLogLevel)
	{
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	if (NULL == APie::CtxSingleton::get().getLogThread())
	{
		pieLogRaw(file, cycle, level, msg, false);
		return;
	}

	APie::LogCmd* ptrCmd = new APie::LogCmd;
	ptrCmd->sFile = file;
	ptrCmd->iCycle = cycle;
	ptrCmd->iLevel = level;
	ptrCmd->sMsg = msg;
	ptrCmd->bIgnoreMore = false;

	APie::Command cmd;
	cmd.type = APie::Command::async_log;
	cmd.args.async_log.ptrData = ptrCmd;
	APie::CtxSingleton::get().getLogThread()->push(cmd);
}

void asyncPieLogIgnoreMerge(const char* file, int cycle, int level, const char *fmt, ...)
{
	va_list ap;
	char msg[PIE_MAX_LOGMSG_LEN] = { '\0' };

	int iConfigLogLevel = APie::CtxSingleton::get().yamlAs<int>({ "log","level" }, 2);
	if ((level & 0xff) < iConfigLogLevel)
	{
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	if (NULL == APie::CtxSingleton::get().getLogThread())
	{
		pieLogRaw(file, cycle, level, msg, true);
		return;
	}

	APie::LogCmd* ptrCmd = new APie::LogCmd;
	ptrCmd->sFile = file;
	ptrCmd->iCycle = cycle;
	ptrCmd->iLevel = level;
	ptrCmd->sMsg = msg;
	ptrCmd->bIgnoreMore = true;

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
	//std::string sLogPrefix = "app";

#ifdef WIN32
	sFile = sCurPath;
	sFile.append("/logs/");
	sFile.append(file);
#else
	sFile = "/usr/local/apie/logs/";
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
	strftime(filePostfix, sizeof(filePostfix), "backup-%Y%m%d-%H%M", localtime(&now));
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

			std::string toDir = APie::CtxSingleton::get().yamlAs<std::string>({"log","backup"}, "");
#ifdef WIN32
			toDir = "D:/backup";
#endif
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
		int32_t iSize = APie::CtxSingleton::get().yamlAs<int>({ "log","split_size" }, 128);
		if (statInfo.st_size > iSize *1240 * 1240)
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

//void fatalExit(const char* message)
//{
//	fprintf(stderr, "%s: %s\n", "fatalExit", message);
//	pieLog("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s\n", "fatalExit", message);
//
//	//exit(EXIT_FAILURE);
//	abort();
//}
