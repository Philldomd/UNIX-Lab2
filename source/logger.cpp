#include "logger.h"

#include <syslog.h>

#include <cstdio>
#include <cstdarg>

int Log::facility = 0;

void Log::open(const char* ident, int option, int facility)
{
	Log::facility = facility;
	openlog(ident, option, facility);
}

void Log::err(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	helpLog(LOG_ERR | facility, format, arglist);
	va_end(arglist);
}

void Log::debug(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	helpLog(LOG_DEBUG | facility, format, arglist);
	va_end(arglist);
}

void Log::info(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	helpLog(LOG_INFO | facility, format, arglist);
	va_end(arglist);
}

void Log::helpLog(int priority, const char* format, va_list ap)
{
	char buffer[512];
	vsnprintf(buffer, sizeof(buffer), format, ap);
	
	syslog(priority, "%s", buffer);
}
