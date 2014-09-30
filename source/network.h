#pragma once

#include <cstdint>

class Network
{
private:
	int acceptSocket;

public:
	Network(uint16_t portNr);
	void startListen();
	void shutdown();
};
