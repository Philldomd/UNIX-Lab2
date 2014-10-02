#include "config.h"
#include "daemon.h"
#include "logger.h"
#include "network.h"

#include <cstdlib>
#include <cstdio>
#include <syslog.h>
#include <unistd.h>

int main(int argc,char* argv[])
{
	Config conf("conf.yaml");
	
	int opt;
	bool daemon = false;
	uint16_t port = conf.getPortNr();
	char* logFile = nullptr;
	while((opt = getopt(argc, argv, "p:dl:")) != -1)
	{	
		switch (opt)
		{
		case 'p':
		{
			int p = atoi(optarg);
			if(p < 0 || p > 65535)
			{
				fprintf(stderr, "Invalid port number\n");
				exit(1);
			}
			port = p;
			break;
		}	
		case 'd':
			daemon = true;
			break;
			
		case 'l':
			logFile = optarg;
			break;
		}
	}
	printf("Port: %hu\n", port);
	printf("Root: %s\n", conf.getRootPath());
	
	Network net(port);
	
	if(daemon)
	{
		daemonizeAndClearFiles();
		Log::open(argv[0], LOG_CONS, LOG_DAEMON);
		Log::info("Daemon server started at port: %hu", port);
	}
	else
	{
		Log::open(argv[0], LOG_CONS | LOG_PERROR, LOG_USER);
		Log::info("Server started at port: %hu", port);
	}
	
	net.startListen();

	Log::info("Server shutting down");
	
	net.shutdown();
	
	return 0;
}

