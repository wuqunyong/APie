#ifndef __PIE_WINDOWS_H_INCLUDED__
#define __PIE_WINDOWS_H_INCLUDED__

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <io.h>
#include <tchar.h>
#include <process.h>
#endif

#endif
