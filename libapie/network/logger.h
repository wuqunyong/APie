#pragma once

#include <stdio.h>
#include <string>
#include <map>

#include "ctx.h"

/* Log levels */
#define PIE_DEBUG 0
#define PIE_VERBOSE 1
#define PIE_NOTICE 2
#define PIE_WARNING 3
#define PIE_ERROR 4
#define PIE_PANIC 5

#define PIE_CYCLE_MINUTE 0
#define PIE_CYCLE_HOUR 1
#define PIE_CYCLE_DAY 2

#define PIE_MAX_LOGMSG_LEN    1024*256 /* Default maximum length of syslog messages */


struct LogFile
{
	FILE * pFile;
	std::string sFile;
	int iCycle;
	int iCreateYear;
	int iCreateMonth;
	int iCreateDay;
	int iCreateHour;
	int iCreateMinute;
}; 

extern std::map<std::string, LogFile*> cacheMap;

std::string getLogLevelName(int level);
void pieLogRaw(const char* file, int cycle, int level, const char* msg, bool ignoreMerge);
void pieLog(const char* file, int cycle, int level, const char *fmt, ...);
void asyncPieLog(const char* file, int cycle, int level, const char *fmt, ...);
void asyncPieLogIgnoreMerge(const char* file, int cycle, int level, const char *fmt, ...);

LogFile* openFile(std::string file, int cycle);
void closeFile(LogFile* ptrFile);
void moveFile(std::string from, std::string to);
void checkRotate();

LogFile* getCacheFile(std::string file, int cycle);
bool isChangeFile(LogFile* ptrFile, int cycle);

void logFileClose();

//void fatalExit(const char* message);


#ifdef WIN32
#define PANIC_ABORT(format, ...) do { \
	std::string formatStr("%s:%d|"); \
	formatStr = formatStr + format; \
	pieLog("PANIC/PANIC", PIE_CYCLE_HOUR, PIE_PANIC, formatStr.c_str(), __FILE__, __LINE__, __VA_ARGS__); \
	abort(); \
} while (0);
#else
#define PANIC_ABORT(format, args...) do { \
	std::string formatStr("%s:%d|"); \
	formatStr = formatStr + format; \
	pieLog("PANIC/PANIC", PIE_CYCLE_HOUR, PIE_PANIC, formatStr.c_str(), __FILE__, __LINE__, ##args); \
	abort(); \
} while (0);
#endif


#ifdef WIN32
#define PIE_LOG(file, cycle, level, format, ...) do { \
    bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		pieLog(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, __VA_ARGS__); \
	} \
	else \
	{ \
		pieLog(file, cycle, level, format, __VA_ARGS__); \
	} \
} while (0);
#else
#define PIE_LOG(file, cycle, level, format, args...) do { \
	bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		pieLog(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, ##args); \
	} \
	else \
	{ \
		pieLog(file, cycle, level, format, ##args); \
	} \
} while (0);
#endif


#ifdef WIN32
#define ASYNC_PIE_LOG(file, cycle, level, format, ...) do { \
	bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		asyncPieLog(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, __VA_ARGS__); \
	} \
	else \
	{ \
		asyncPieLog(file, cycle, level, format, __VA_ARGS__); \
	} \
} while (0);
#else
#define ASYNC_PIE_LOG(file, cycle, level, format, args...) do { \
	bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		asyncPieLog(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, ##args); \
	} \
	else \
	{ \
		asyncPieLog(file, cycle, level, format, ##args); \
	} \
} while (0);
#endif

#ifdef WIN32
#define ASYNC_PIE_LOG_CUSTOM(file, cycle, level, format, ...) do { \
	bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		asyncPieLogIgnoreMerge(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, __VA_ARGS__); \
	} \
	else \
	{ \
		asyncPieLogIgnoreMerge(file, cycle, level, format, __VA_ARGS__); \
	} \
} while (0);
#else
#define ASYNC_PIE_LOG_CUSTOM(file, cycle, level, format, args...) do { \
	bool bShowPos = APie::CtxSingleton::get().getConfigs()->log.show_pos; \
	if (bShowPos) \
	{ \
		std::string formatStr("%s:%d|"); \
		formatStr = formatStr + format; \
		asyncPieLogIgnoreMerge(file, cycle, level, formatStr.c_str(), __FILE__, __LINE__, ##args); \
	} \
	else \
	{ \
		asyncPieLogIgnoreMerge(file, cycle, level, format, ##args); \
	} \
} while (0);
#endif


template <typename ...Args>
std::string SSImpl(Args&& ...args)
{
	std::ostringstream ss;

	(ss << ... << std::forward<Args>(args));

	return ss.str();
}