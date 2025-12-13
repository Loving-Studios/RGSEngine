#include "Log.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

// Initialize the empty buffer for the console
std::string Log::logBuffer = "";

void log(const char file[], int line, const char* format, ...)
{
    static char tmp_string[4096];
    static char tmp_string2[4096];
    static va_list  ap;

    // Construct the string from variable arguments
    va_start(ap, format);
    vsprintf_s(tmp_string, 4096, format, ap);
    va_end(ap);

    // Output window of Visual Studio
    sprintf_s(tmp_string2, 4096, "\n%s(%d) : %s", file, line, tmp_string);
    OutputDebugStringA(tmp_string2);

    // Construct the final log message
    Log::logBuffer += std::string(tmp_string) + "\n";
}