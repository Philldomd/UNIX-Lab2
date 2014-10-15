#include "network.h"

#include "http.h"
#include "logger.h"
#include "MimeFinder.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

Network::Network(uint16_t portNr, MimeFinder* mimeFinder)
	: mimeFinder(mimeFinder)
{
	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;	
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	
	char buf[8];
	snprintf(buf, 8, "%hu", portNr);
	printf("Binding to port: %s\n", buf);
	
	addrinfo* result = nullptr;
	int err = getaddrinfo(nullptr, buf, &hints, &result);
	if(err != 0)
	{
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(err));
		exit(1);
	}
	
	int sock;
	addrinfo*rp;
	for(rp = result; rp != nullptr; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock == -1)
		{
			fprintf(stderr, "socket(): %s\n", strerror(errno));
			continue;
		}
		int boolean = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &boolean, sizeof(boolean)) != 0)
		{
			fprintf(stderr, "setsockopt(): %s\n", strerror(errno));
			break;
		}
		if(bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		else
		{
			char hostBuf[NI_MAXHOST];
			char servBuf[NI_MAXSERV];
			
			if((err = getnameinfo(rp->ai_addr, rp->ai_addrlen, hostBuf, sizeof(hostBuf), servBuf, sizeof(servBuf), NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
			{
				fprintf(stderr, "getnameinfo(): %s\n", gai_strerror(err));
				exit(1);
			}
			
			fprintf(stderr, "Bind: [%s] : %s %s\n", hostBuf, servBuf, strerror(errno));
		}
		
		if(close(sock) == -1)
		{
			perror("close()");
		}
	}
	
	if(rp == nullptr)
	{
		fprintf(stderr, "Could not bind connection\n");
		exit(1);
	}
	
	char hostBuf[NI_MAXHOST];
	char servBuf[NI_MAXSERV];
	
	if((err = getnameinfo(rp->ai_addr, rp->ai_addrlen, hostBuf, sizeof(hostBuf), servBuf, sizeof(servBuf), NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
	{
		fprintf(stderr, "getnameinfo(): %s\n", gai_strerror(err));
		exit(1);
	}
	printf("Bound socket at: [%s] : %s \n", hostBuf, servBuf);
	
	freeaddrinfo(result);
	
	acceptSocket = sock;
}

const char* getStatusStr(RequestErr status)
{
	switch (status)
	{
	case RequestErr::OK:
		return "OK";
		
	case RequestErr::BAD_REQUEST:
		return "Bad request";
		
	case RequestErr::FORBIDDEN:
		return "Forbidden";
		
	case RequestErr::NOT_FOUND:
		return "Not found";
		
	case RequestErr::INTERNAL:
		return "Internal server error";
		
	case RequestErr::NOT_IMPLEMENTED:
		return "Not implemented";
		
	default:
		return "Unknown status";
	}
}

Network::Err Network::startListen()
{
	if (listen(acceptSocket, 1) == -1)
	{
		Log::err("listen(): %s", strerror(errno));
		return Err::Listen;
	}
	
	return Err::OK;
}

int Network::accept()
{
	return ::accept(acceptSocket, nullptr, nullptr);
}

void Network::handleConnection(int connSock)
{
	char buffer[1024];
	ssize_t receivedBytes = recv(connSock, buffer, 1024, 0);
	if (receivedBytes == -1)
	{
		Log::err("recv(): %s", strerror(errno));
		if (close(connSock) == -1)
		{
			Log::err("close(): %s", strerror(errno));
		}
		return;
	}
	else if (receivedBytes == 0)
	{
		Log::info("Closing disconnected socket");
		if (close(connSock) == -1)
		{
			Log::err("close(): %s", strerror(errno));
		}
		return;
	}
	
	char* rPos = (char*)memchr(buffer, '\r', receivedBytes);
	char* nPos = (char*)memchr(buffer, '\n', receivedBytes);
	
	char* lineEndPos = nPos;
	if (nPos == nullptr || (rPos != nullptr && rPos < nPos))
	{
		lineEndPos = rPos;
	}
	
	RequestResult res;
	RequestErr status;
	size_t requestLineLen;
	if (lineEndPos == nullptr)
	{
		requestLineLen = receivedBytes;
		status = RequestErr::BAD_REQUEST;
		res.version = HttpVersion::V1_0;
	}
	else
	{
		requestLineLen = lineEndPos - buffer;
		status = readRequestLine(buffer, requestLineLen, res);
	}
	
	char pathBuff[1024];
	const char* path = nullptr;
	
	switch (status)
	{
	case RequestErr::OK:
		{
			int unescapedLen = unescape(res.path, pathBuff, res.pathLen);
			if (unescapedLen == -1)
			{
				status = RequestErr::BAD_REQUEST;
				path = "/400.html";
			}
			else
			{						
				pathBuff[unescapedLen] = 0;
				path = pathBuff;
			}
		}
		break;
		
	case RequestErr::BAD_REQUEST:
		path = "/400.html";
		break;
		
	case RequestErr::FORBIDDEN:
		path = "/403.html";
		break;
		
	case RequestErr::NOT_FOUND:
		path = "/404.html";
		break;
		
	case RequestErr::INTERNAL:
		path = "/500.html";
		break;
		
	case RequestErr::NOT_IMPLEMENTED:
		path = "/501.html";
		break;
	}
	
	if (strcmp(path, "/") == 0)
	{
		path = "/index.html";
	}
	
	int file = open(path, O_RDONLY);
	if (file == -1)
	{
		status = RequestErr::NOT_FOUND;
		file = open("/404.html", O_RDONLY);
		
		if (file == -1)
		{
			Log::err("open(): %s, Failed to open 404.html", strerror(errno));
			
			if (close(connSock) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			return;
		}
	}
		
	struct stat fileStat;
	if (fstat(file, &fileStat) == -1)
	{
		Log::err("fstat(): %s", strerror(errno));
		
		if (close(file) == -1)
		{
			Log::err("close(): %s", strerror(errno));
		}
		
		if (close(connSock) == -1)
		{
			Log::err("close(): %s", strerror(errno));
		}
		
		return;
	}
	
	if (res.version >= HttpVersion::V1_0)
	{
		char headerBuff[1024];
		int headerLen = snprintf(headerBuff, sizeof(headerBuff), "HTTP/1.0 %3.3d %s\r\n", (int)status, getStatusStr(status));
		
		const char* mimeType = mimeFinder->findMimeType(dup(file), path);
		if (lseek(file, 0, SEEK_SET) == -1)
		{
			Log::err("lseek(): %s", strerror(errno));
			
			if (close(file) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			if (close(connSock) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			return;
		}
		if (mimeType != nullptr)
		{
			headerLen += snprintf(headerBuff + headerLen, sizeof(headerBuff) - headerLen,
				"Content-Type: %s\r\n", mimeType);
		}
		
		headerLen += snprintf(headerBuff + headerLen, sizeof(headerBuff) - headerLen, "Content-Length: %ld\r\n\r\n", fileStat.st_size);
		
		if (send(connSock, headerBuff, headerLen, 0) == -1)
		{
			Log::err("send(): %s", strerror(errno));
			
			if (close(file) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			if (close(connSock) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			return;
		}
	}
	
	size_t bytesSent;
	if(res.method == HttpMethod::GET)
	{
		if ((bytesSent = sendfile(connSock, file, nullptr, fileStat.st_size)) == -1)
		{
			Log::err("sendfile(): %s", strerror(errno));
			
			if (close(file) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			if (close(connSock) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			
			return;
		}
	}
	
	if(close(file) == -1)
	{
		Log::err("close(): %s", strerror(errno));
	}
	
	logStatus(buffer, requestLineLen, connSock, bytesSent, (int)status);
	
	if (close(connSock) == -1)
	{
		Log::err("close(): %s", strerror(errno));
	}
}

void Network::shutdown()
{
	if(close(acceptSocket) == -1)
	{
		Log::err("close(): %s", strerror(errno));
	}
}

void Network::logStatus(char* buffer, size_t bufflen, int fd, size_t bytesSent, int status)
{
	int error;
	sockaddr_storage addrstorage;
	socklen_t socklen = sizeof(addrstorage);
	
	error = getpeername(fd, (sockaddr*)&addrstorage, &socklen);
	if(error == -1)
	{
		Log::err("getnameinfo(): %s", strerror(errno));
		return;
	}
	
	char namebuff[NI_MAXHOST];
	error = getnameinfo((sockaddr*)&addrstorage, sizeof(addrstorage), namebuff, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	if(error != 0)
	{
		Log::err("getnameinfo(): %s", gai_strerror(error));
		return;
	}
	
	buffer[bufflen] = '\0';
	
	Log::connection(namebuff, "-", buffer, status, bytesSent);
}
