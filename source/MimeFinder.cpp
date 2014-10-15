#include "MimeFinder.h"

#include "logger.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
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
		fprintf(stderr, "magic_open(): %s\n", strerror(errno));
		exit(1);
	}
	
	if (magic_load(cookie, nullptr) == -1)
	{
		fprintf(stderr, "magic_load(): %s\n", magic_error(cookie));
		exit(1);
	}
	
	if (magic_compile(cookie, nullptr) == -1)
	{
		fprintf(stderr, "magic_compile(): %s\n", magic_error(cookie));
		exit(1);
	}
	
	pthread_mutex_init(&lock, NULL);
}

void MimeFinder::shutdown()
{
	magic_close(cookie);
	cookie = nullptr;
}

static const char* mimeTypes[][2] =
{
	{ "html", "text/html" },
	{ "css", "text/css" },
	{ "js", "application/javascript" },
};

static const char* findFromName(const char* filename)
{
	if (!filename)
	{
		return nullptr;
	}
	
	const char* ext = strrchr(filename, '.');
	if (ext == NULL)
	{
		return nullptr;
	}
	
	ext++;
	
	for (auto map : mimeTypes)
	{
		if (strcmp(map[0], ext) == 0)
		{
			return map[1];
		}
	}
	
	return nullptr;
}

const char* MimeFinder::findMimeType(int fd, const char* filename)
{
	const char* ret = findFromName(filename);
	if (ret != nullptr)
	{
		close(fd);
		return ret;
	}
	
	int err;
	if ((err = pthread_mutex_lock(&lock)) != 0)
	{
		Log::err("pthread_mutex_lock(): %s", strerror(err));
		return nullptr;
	}
	
	ret = magic_descriptor(cookie, fd);
	
	if ((err = pthread_mutex_unlock(&lock)) != 0)
	{
		Log::err("pthread_mutex_unlock(): %s", strerror(err));
	}
	
	if (ret == nullptr)
	{
		Log::err("magic_descriptor(): %s", magic_error(cookie));
	}
	
	return ret;
}
