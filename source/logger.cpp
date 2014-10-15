#include "logger.h"

#include <syslog.h>

#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <ctime>

int Log::facility = 0;
pthread_mutex_t Log::lock;

bool Log::logToFile = false;
FILE *Log::logFile = NULL;
FILE *Log::errFile = NULL;

void Log::openFile(const char* path)
{
	logToFile = true;
	char logPath[512] = {};
	strncat(logPath, path, strlen(path));
	strncat(logPath, ".log", 4);
	char errPath[512] = {};
	strncat(errPath, path, strlen(path));
	strncat(errPath, ".err", 4);
	logFile = fopen(logPath, "a");
	if(logFile == NULL)
	{
		printf("Error fopen(): %s", strerror(errno));
	}
	errFile = fopen(errPath, "a");
	if(logFile == NULL)
	{
		printf("Error fopen(): %s", strerror(errno));
	}
}

void Log::open(const char* ident, int option, int facility)
{
	logToFile = false;
	Log::facility = facility;
	openlog(ident, option, facility);
	
	pthread_mutex_init(&lock, NULL);
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
	int length = vsnprintf(buffer, sizeof(buffer), format, ap);

	pthread_mutex_lock(&lock);
	
	if(logToFile)
	{
		if((priority & (~facility)) == LOG_INFO)
		{
			fwrite(buffer, 1, length, logFile);
			fwrite("\n", 1, 1, logFile);
			fflush(logFile);
		}
		else 
		{
			fwrite(buffer, 1, length, errFile);
			fwrite("\n", 1, 1, errFile);
			fflush(errFile);
		}
	}
	else
	{
		syslog(priority, "%s", buffer);
	}
	
	pthread_mutex_unlock(&lock);
}
