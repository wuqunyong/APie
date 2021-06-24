#include "exception_trap.h"

#include "../network/windows_platform.h"
#include "../network/logger.h"

#ifdef WIN32
#include "../apie.h"
#include <ImageHlp.h>

#pragma comment( lib, "dbghelp.lib" )

const char* CUT_LINE = "\n________________________________________\n\n";

std::string GetStackDetails(CONTEXT* pContext)
{
	if (pContext == NULL)
	{
		return "Stack: CONTEXT NULL\n";
	}

	char szBuffer[512] = { '\0' };
	std::string strDetails;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

	SymSetOptions(SYMOPT_DEFERRED_LOADS);
	if (SymInitialize(hProcess, NULL, TRUE) == FALSE)
	{
		return "Stack: SymInitialize False\n";
	}

	strDetails += "Call Stack:\n";
	strDetails += "Address\t\tFrame\t\tFunction\n";

	//要把pContext的内容取出来
	//调用StackWalk,可能会修改CONTEXT的内容,导致dmp文件无法输出详细堆栈
	CONTEXT TempContext = *pContext;

	STACKFRAME StackFrame;
	memset(&StackFrame, 0, sizeof(StackFrame));
#if defined(_AMD64_)
	StackFrame.AddrPC.Offset = TempContext.Rip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = TempContext.Rsp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = TempContext.Rbp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_X86_)
	StackFrame.AddrPC.Offset = TempContext.Eip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = TempContext.Esp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = TempContext.Ebp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
#endif

	while (true)
	{
		// Get the next stack frame.
		if (StackWalk(dwMachineType, hProcess, hThread, &StackFrame, &TempContext, 0, SymFunctionTableAccess, SymGetModuleBase, 0) == FALSE)
		{
			break;
		}

		// Basic sanity check to make sure the frame is OK.
		// Bail if not.
		if (StackFrame.AddrFrame.Offset == 0)
		{
			break;
		}

		_snprintf(szBuffer, sizeof(szBuffer) - 1, "%I64X\t%I64X\t", StackFrame.AddrPC.Offset, StackFrame.AddrFrame.Offset);
		strDetails += szBuffer;

		// Get the source line for this stack frame entry.
		IMAGEHLP_LINE ImageHlpLine = { sizeof(IMAGEHLP_LINE) };
		DWORD dwLineDisplacement;
		if (SymGetLineFromAddr(hProcess, StackFrame.AddrPC.Offset, &dwLineDisplacement, &ImageHlpLine) == TRUE)
		{
			_snprintf(szBuffer, sizeof(szBuffer) - 1, "%s line %u", ImageHlpLine.FileName, ImageHlpLine.LineNumber);
			strDetails += szBuffer;
		}

		strDetails += "\n";
	}

	SymCleanup(hProcess);

	return strDetails;
}

bool GetLogicalAddress(void* pAddress, char* pszModule, DWORD dwSize, DWORD& dwSection, DWORD& dwOffset)
{
	MEMORY_BASIC_INFORMATION MemoryBasicInformation;
	if (VirtualQuery(pAddress, &MemoryBasicInformation, sizeof(MemoryBasicInformation)) == 0)
		return false;

	HMODULE hModule = (HMODULE)MemoryBasicInformation.AllocationBase;
	if (GetModuleFileNameA(hModule, pszModule, dwSize) == 0)
		return false;

	// Point to the DOS header in memory
	IMAGE_DOS_HEADER* pDosHeader = (PIMAGE_DOS_HEADER)hModule;

	// From the DOS header, find the NT (PE) header
	IMAGE_NT_HEADERS* pNtHeaders = (PIMAGE_NT_HEADERS)(hModule + pDosHeader->e_lfanew);

	IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);

	// RVA(相对虚拟地址)
	// RVA is offset from module load address
	DWORD dwRVA = (DWORD)pAddress - (DWORD)hModule;

	// Iterate through the section table, looking for the one that encompasses the linear address.
	for (unsigned int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++, pSectionHeader++)
	{
		DWORD dwSectionStart = pSectionHeader->VirtualAddress;
		DWORD dwSectionEnd = dwSectionStart + std::max(pSectionHeader->SizeOfRawData, pSectionHeader->Misc.VirtualSize);

		// Is the address in this section?
		if ((dwRVA >= dwSectionStart) && (dwRVA <= dwSectionEnd))
		{
			// Yes, address is in the section.  Calculate section and offset,
			// and store in the "section" & "offset" params, which were passed by reference.
			dwSection = i + 1;
			dwOffset = dwRVA - dwSectionStart;
			return true;
		}
	}

	// Should never get here!
	return false;
}

std::string GetExceptionString(DWORD dwCode)
{
	switch (dwCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:			return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_DATATYPE_MISALIGNMENT:		return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_BREAKPOINT:					return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_SINGLE_STEP:					return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:		return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_FLT_DENORMAL_OPERAND:		return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:			return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT:			return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION:		return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW:				return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK:				return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW:				return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:			return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW:				return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_PRIV_INSTRUCTION:			return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR:				return "EXCEPTION_IN_PAGE_ERROR";
	case EXCEPTION_ILLEGAL_INSTRUCTION:			return "EXCEPTION_ILLEGAL_INSTRUCTION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:	return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	case EXCEPTION_STACK_OVERFLOW:				return "EXCEPTION_STACK_OVERFLOW";
	case EXCEPTION_INVALID_DISPOSITION:			return "EXCEPTION_INVALID_DISPOSITION";
	case EXCEPTION_GUARD_PAGE:					return "EXCEPTION_GUARD_PAGE";
	case EXCEPTION_INVALID_HANDLE:				return "EXCEPTION_INVALID_HANDLE";
	case CONTROL_C_EXIT:						return "CONTROL_C_EXIT";
	default:									break;
	}

	// If not one of the "known" exceptions, try to get the string
	// from NTDLL.DLL's message table.

	HMODULE hNTDLL = GetModuleHandleA("NTDLL.DLL");
	if (hNTDLL == NULL)
	{
		return "NTDLL"; //未取得NTDLL.DLL模块
	}

	char szBuffer[512] = { '\0' };
	FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, hNTDLL, dwCode, 0, szBuffer, sizeof(szBuffer), NULL);

	int iLength = (int)strlen(szBuffer);
	if (szBuffer[iLength - 1] == '\n')
	{
		szBuffer[iLength - 1] = '\0';
	}

	return szBuffer;
}

void SaveLog(EXCEPTION_POINTERS* pExceptionPointers)
{
	char szBuffer[512] = { '\0' };
	std::string strException;

	//时间
	time_t tCurrentTime;
	time(&tCurrentTime);
	struct tm* pTimeInfo;
	pTimeInfo = localtime(&tCurrentTime);
	strftime(szBuffer, sizeof(szBuffer), "Exception Time: %Y-%m-%d %H:%M:%S\n", pTimeInfo);
	strException += szBuffer;

	//进程ID,线程ID
	DWORD dwProcessID = GetCurrentProcessId();
	DWORD dwThreadID = GetCurrentThreadId();
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Process ID=%d, Thread ID=%d\n\n", dwProcessID, dwThreadID);
	strException += szBuffer;

	//异常代码
#ifdef _WIN64
	EXCEPTION_RECORD64* pExceptionRecord = (EXCEPTION_RECORD64*)pExceptionPointers->ExceptionRecord;
#else
	EXCEPTION_RECORD32* pExceptionRecord = (EXCEPTION_RECORD32*)pExceptionPointers->ExceptionRecord;
#endif
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Exception Code: %lX %s\n", pExceptionRecord->ExceptionCode, GetExceptionString(pExceptionRecord->ExceptionCode).c_str());
	strException += szBuffer;

	//错误地址、模块
	char szModule[256] = { '\0' };
	DWORD dwSection = 0, dwOffset = 0;
	GetLogicalAddress((void*)pExceptionRecord->ExceptionAddress, szModule, sizeof(szModule), dwSection, dwOffset);
#ifdef _WIN64
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Fault Address: %I64X %02X:%lX\nFile: %s\n\n", pExceptionRecord->ExceptionAddress, dwSection, dwOffset, szModule);
#else
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Fault Address: %08X %02X:%08X\nFile: %s\n\n", pExceptionRecord->ExceptionAddress, dwSection, dwOffset, szModule);
#endif
	strException += szBuffer;

	//寄存器
	strException += "Registers:\n";
	CONTEXT* pContext = pExceptionPointers->ContextRecord;
#if defined(_AMD64_)
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "RAX:%I64X  RBX:%I64X\r\n", pContext->Rax, pContext->Rbx);							strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "RCX:%I64X  RDX:%I64X\r\n", pContext->Rcx, pContext->Rdx);							strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "RSI:%I64X  RDI:%I64X\r\n", pContext->Rsi, pContext->Rdi);							strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "RIP:%I64X  RSP:%I64X  RBP:%I64X\n", pContext->Rip, pContext->Rsp, pContext->Rbp);	strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "CS:%X  DS:%X  ES:%X  FS:%X  GS:%X  SS:%X\n", pContext->SegCs, pContext->SegDs, pContext->SegEs, pContext->SegFs, pContext->SegGs, pContext->SegSs);
	strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Flags:%08X\n\n", pContext->EFlags);
	strException += szBuffer;
#elif defined(_X86_)
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "EAX:%08X  EBX:%08X\r\n", pContext->Eax, pContext->Ebx);								strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "ECX:%08X  EDX:%08X\r\n", pContext->Ecx, pContext->Edx);								strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "ESI:%08X  EDI:%08X\r\n", pContext->Esi, pContext->Edi);								strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "EIP:%08X  ESP:%08X  EBP:%08X\n", pContext->Eip, pContext->Esp, pContext->Ebp);		strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "CS:%X  DS:%X  ES:%X  FS:%X  GS:%X  SS:%X\n", pContext->SegCs, pContext->SegDs, pContext->SegEs, pContext->SegFs, pContext->SegGs, pContext->SegSs);
	strException += szBuffer;
	_snprintf(szBuffer, sizeof(szBuffer) - 1, "Flags:%08X\n\n", pContext->EFlags);
	strException += szBuffer;
#endif

	//调试信息、代码行
	strException += GetStackDetails(pContext);

	//写入Error.log
	FILE* pFile = fopen("Error.log", "at+");
	if (pFile == NULL)
	{
		return;
	}

	fwrite(CUT_LINE, strlen(CUT_LINE), 1, pFile);
	fwrite(strException.c_str(), strException.length(), 1, pFile);
	fclose(pFile);
}

void CreateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
	//时间
	SYSTEMTIME stLocalTime;
	GetLocalTime(&stLocalTime);

	//创建dump文件
	char szFileName[MAX_PATH] = { '\0' };
	HANDLE hDumpFile = NULL;
	_snprintf(szFileName, sizeof(szFileName) - 1, "Crash_%04d-%02d-%02d_%02d-%02d-%02d.dmp", stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);
	hDumpFile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hDumpFile == NULL)
	{
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
	ExceptionInfo.ThreadId = GetCurrentThreadId();
	ExceptionInfo.ExceptionPointers = pExceptionPointers;
	ExceptionInfo.ClientPointers = TRUE;

	MINIDUMP_TYPE MiniDumpWithDataSegs = MINIDUMP_TYPE(MiniDumpNormal
		| MiniDumpWithHandleData
		| MiniDumpWithUnloadedModules
		| MiniDumpWithIndirectlyReferencedMemory
		| MiniDumpScanMemory
		| MiniDumpWithProcessThreadData
		| MiniDumpWithFullMemory
		| MiniDumpWithProcessThreadData);

	//写入dump文件
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, &ExceptionInfo, NULL, NULL);
	CloseHandle(hDumpFile);
}

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* p)
{
	SaveLog(p);
	CreateDump(p);
	//return EXCEPTION_CONTINUE_SEARCH;
	return EXCEPTION_EXECUTE_HANDLER;
}

namespace APie {
	void ExceptionTrap()
	{
		SetUnhandledExceptionFilter(ExceptionFilter);
	}
}

#else

namespace APie {
	std::string generateBackTrace()
	{
		int maxFrames = 128;
		void* frames[128];
		int numFrames = backtrace(frames, maxFrames);
		char** symbols = backtrace_symbols(frames, numFrames);
		if (!symbols)
		{
			return "";
		}

		std::ostringstream ss;
		ss << "Stack:\n";
		for (int i = 1; i < numFrames; ++i) {
			//ss << symbols[i] << '\n';
			ss << symbols[i];
			if (i + 1 < numFrames)
			{
				ss << '\n';
			}
		}
		free(symbols);

		return ss.str();
	}

	void sigsegvHandler(int sig, siginfo_t *info, void *secret)
	{
		/* Log the stack trace */
		pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "------ STACK TRACE BEGIN ------");
		std::string traceStr = generateBackTrace();
		pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "sigaction: Got signal %d|%s|bt:%s", sig, strsignal(sig), traceStr.c_str());
		pieLog("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "------ STACK TRACE END ------");

		/* Make sure we exit with the right signal at the end. So for instance
		* the core will be dumped if enabled. */
		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
		act.sa_handler = SIG_DFL;
		sigaction(sig, &act, NULL);
		kill(getpid(), sig);
	}

	void setupSignalHandlers(void)
	{
		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
		act.sa_sigaction = sigsegvHandler;
		sigaction(SIGINT, &act, NULL);
		sigaction(SIGSEGV, &act, NULL);
		sigaction(SIGBUS, &act, NULL);
		sigaction(SIGFPE, &act, NULL);
		sigaction(SIGILL, &act, NULL);
	}

	void ExceptionTrap()
	{
		setupSignalHandlers();
	}
}

#endif