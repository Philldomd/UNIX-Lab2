#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


Config::Config(const char* filePath)
{
	FILE* stream;
	if((stream = fopen(filePath, "r")) == nullptr)
	{
		perror("Config::Config#fopen");
		return;
	}
	
	char* line = nullptr;
	size_t len = 0;
	ssize_t read;
	while ((read = getline(&line, &len, stream)) != -1)
	{
		char addr[1024];
		uint16_t port;
		if(sscanf(line, "root: %1023s", addr) == 1)
		{
			memcpy(rootPath, addr, 1024);  
		}
		else if (sscanf(line, "portNr: %hu\n", &port) == 1)
		{
			portNr = port;
		}
		else
		{
			printf("Unrecognized option: %s\n", line);
		}
	}
	
	free(line);
	fclose(stream);
}

uint16_t Config::getPortNr() const
{
	return portNr;
}

const char* Config::getRootPath() const
{
	return rootPath;
}
