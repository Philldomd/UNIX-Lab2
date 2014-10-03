#pragma once

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
};
