#include "network.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Network::Network(uint16_t portNr)
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
	
	if( close(sock) == -1)
	{
		perror("close()");
	}
}
