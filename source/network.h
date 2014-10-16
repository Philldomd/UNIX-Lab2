#pragma once

#include <cstddef>
#include <cstdint>

class MimeFinder;

class Network
{
public:
	enum class Err
	{
		OK = 0,
		Listen,
		Accept,
		Setup,
		Process,
		ReadMore,
	};
	
	const static size_t readBufSize = 1024;
	struct SocketData
	{
		int fd;
		char readBuf[readBufSize];
		size_t readLen;
	};
	
private:
	int acceptSocket;
	MimeFinder* mimeFinder;

public:
	Network(uint16_t portNr, MimeFinder* mimeFinder);
	Err startListen();
	int accept();
	Err handleConnection(SocketData* conn);
	void shutdown();
	
	int getAcceptSocket() const { return acceptSocket; }
	
private:
	void logStatus(char* buffer, size_t bufflen, int fd, size_t bytesSent, int status);
};
