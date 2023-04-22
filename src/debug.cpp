#include <windows.h>
#include <stdio.h>
#include "debug.h"

void OutputMessage(const char* format, ...)
{
	const int bufLen{ 512 };
	char buffer[bufLen];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, bufLen, format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

void OutputMessage(const wchar_t* format, ...)
{
	const int bufLen{ 512 };
	wchar_t buffer[bufLen];
	va_list args;
	va_start(args, format);
	vswprintf(buffer, bufLen, format, args);

	OutputDebugStringW(buffer);

	va_end(args);
}