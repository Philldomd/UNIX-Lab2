#pragma once

#include <cstdarg>
#include <cstddef>

class Log
{
private:
	static int facility;
	
public:
	static void open(const char* ident, int option, int facility);

	static void err(const char* format, ...);
	static void debug(const char* format, ...);
	static void info(const char* format, ...);
	
	static void connection(const char* client, const char* user,
		const char* request, int status, size_t bytesSent);
	
private:
	static void helpLog(int priority, const char* format, va_list ap);
};
