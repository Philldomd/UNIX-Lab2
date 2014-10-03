#include "network.h"

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
#include <sys/types.h>
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

void Network::startListen()
{
	if (listen(acceptSocket, 1) == -1)
	{
		Log::err("listen(): %s", strerror(errno));
		return;
	}

	while (true)
	{
		int acceptedSocket;
		if ((acceptedSocket = accept(acceptSocket, nullptr, nullptr)) == -1)
		{
			Log::err("accept(): %s", strerror(errno));
			return;
		}
		
		Log::debug("Got connection");
		
		char buffer[1024];
		ssize_t receivedBytes = recv(acceptedSocket, buffer, 1024, 0);
		if (receivedBytes == -1)
		{
			Log::err("recv(): %s", strerror(errno));
			return;
		}

		Log::debug("Received: %.*s", receivedBytes, buffer);
		
		char* tok1 = strtok(buffer, " ");
		if (strcmp(tok1, "GET") != 0)
		{
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			continue;
		}
		
		char* tok2 = strtok(nullptr, " ");
		if (tok2 == nullptr)
		{
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			continue;
		}
		
		char* tok3 = strtok(nullptr, "\r");
		if (strcmp(tok3, "HTTP/1.1") != 0)
		{
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			continue;
		}
		
		int file;
		if (strcmp(tok2, "/") == 0)
		{
			file = open("/index.html", O_RDONLY);
		}
		else
		{
			file = open(tok2, O_RDONLY);
		}
		
		if (file == -1)
		{
			Log::err("open(): %s", strerror(errno));
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			continue;
		}
		
		const char header[] = "HTTP/1.1 200 OK\r\n";
		if (send(acceptedSocket, header, sizeof(header), 0) == -1)
		{
			Log::err("send(): %s", strerror(errno));
			close(file);
			continue;
		}
		
		const char* mimeType = mimeFinder->findMimeType(dup(file));
		if (lseek(file, 0, SEEK_SET) == -1)
		{
			Log::err("lseek(): %s", strerror(errno));
			close(file);
			continue;
		}
		if (mimeType != nullptr)
		{
			const char cont[] = "Content-Type: ";
			send(acceptedSocket, cont, sizeof(cont), 0);
			send(acceptedSocket, mimeType, strlen(mimeType), 0);
			send(acceptedSocket, "\r\n", 2, 0);
		}
		
		send(acceptedSocket, "\r\n", 2, 0);
		
		struct stat fileStat;
		if (fstat(file, &fileStat) == -1)
		{
			Log::err("fstat(): %s", strerror(errno));
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			close(file);
			continue;
		}
		
		Log::info("Sending %d bytes", fileStat.st_size);
		
		if (sendfile(acceptedSocket, file, nullptr, fileStat.st_size) == -1)
		{
			Log::err("sendfile(): %s", strerror(errno));
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
			close(file);
			continue;
		}
		
		if (close(acceptedSocket) == -1)
		{
			Log::err("close(): %s", strerror(errno));
		}
		
		close(file);
	}
}

void Network::shutdown()
{
	if(close(acceptSocket) == -1)
	{
		Log::err("close(): %s", strerror(errno));
	}
}
