#pragma once

#include <magic.h>

class MimeFinder
{
private:
	magic_t cookie;

public:
	MimeFinder();

	void init();
	void shutdown();
	
	const char* findMimeType(int fd);
};
