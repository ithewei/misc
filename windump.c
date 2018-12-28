#include <stdio.h>
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib,"DbgHelp.lib")

LONG UnhandledException(EXCEPTION_POINTERS *pException) {
    char modulefile[256];
    GetModuleFileName(NULL, modulefile, sizeof(modulefile));
    char* pos = strrchr(modulefile, '\\');
    char* modulefilename = pos + 1;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char filename[256];
    snprintf(filename, sizeof(filename), "core_%s_%04d%02d%02d_%02d%02d%02d_%03d.dump",
        modulefilename,
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    HANDLE hDumpFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char * argv[]) {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)UnhandledException);
    char* p = NULL;
    *p = 0;
    return 0;
}
