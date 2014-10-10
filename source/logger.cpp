#include "logger.h"

#include <syslog.h>

#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>

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

void Log::connection(const char* client, const char* user,
		const char* request, int status, size_t bytesSent)
{
	time_t now = time(NULL);
	if (now == -1)
	{
		Log::err("time(): %s", strerror(errno));
		return;
	}
	tm splitTime;
	if (localtime_r(&now, &splitTime) == nullptr)
	{
		Log::err("localtime_r(): %s", "Failed!");
		return;
	}
	
	char buffer[29];
	size_t len = strftime(buffer, 29, "[%d/%b/%Y:%T %z]", &splitTime);
	if(len != 28)
	{
		Log::err("strftime(): %s", "Incorrect date length");
		return;
	}
	
	if (bytesSent > 0)
	{
		Log::info("%s - %s %s \"%s\" %d %d", client, user, buffer, request, status, bytesSent);
	}
	else
	{
		Log::info("%s - %s %s \"%s\" %d -", client, user, buffer, request, status);
	}
}

void Log::helpLog(int priority, const char* format, va_list ap)
{
	char buffer[512];
	vsnprintf(buffer, sizeof(buffer), format, ap);
	
	syslog(priority, "%s", buffer);
}
