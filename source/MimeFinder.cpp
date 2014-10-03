#include "MimeFinder.h"

#include "logger.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

MimeFinder::MimeFinder()
	: cookie(nullptr)
{
}

void MimeFinder::init()
{
	cookie = magic_open(MAGIC_MIME_TYPE);
	if (cookie == nullptr)
	{
		Log::err("magic_open(): %s", strerror(errno));
		exit(1);
	}
	
	if (magic_load(cookie, nullptr) == -1)
	{
		Log::err("magic_load(): %s", magic_error(cookie));
		exit(1);
	}
	
	if (magic_compile(cookie, nullptr) == -1)
	{
		Log::err("magic_compile(): %s", magic_error(cookie));
		exit(1);
	}
}

void MimeFinder::shutdown()
{
	magic_close(cookie);
	cookie = nullptr;
}

const char* MimeFinder::findMimeType(int fd)
{
	const char* ret = magic_descriptor(cookie, fd);
	if (ret == nullptr)
	{
		Log::err("magic_descriptor(): %s", magic_error(cookie));
	}
	
	return ret;
}
