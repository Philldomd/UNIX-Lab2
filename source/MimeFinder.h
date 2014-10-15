#pragma once

#include <magic.h>

#include <pthread.h>

class MimeFinder
{
private:
	magic_t cookie;
	
	pthread_mutex_t lock;

public:
	MimeFinder();

	void init();
	void shutdown();
	
	const char* findMimeType(int fd, const char* filename);
};
