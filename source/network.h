#pragma once

#include <cstddef>
#include <cstdint>

class MimeFinder;

class Network
{
private:
	int acceptSocket;
	MimeFinder* mimeFinder;

public:
	Network(uint16_t portNr, MimeFinder* mimeFinder);
	void startListen();
	void shutdown();
	
private:
	void logStatus(char* buffer, size_t bufflen, int fd, size_t bytesSent, int status);
};
