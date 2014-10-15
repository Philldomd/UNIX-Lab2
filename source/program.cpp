#include "config.h"
#include "daemon.h"
#include "logger.h"
#include "MimeFinder.h"
#include "network.h"

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#define NOBODY 65534

int main(int argc,char* argv[])
{
	struct sigaction act = {};
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);
		
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
	
	MimeFinder mimeFinder;
	mimeFinder.init();
	
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
	
	Network net(port, &mimeFinder);
	
	if(chroot(conf.getRootPath()) == -1)
	{
		int errnum = errno;
		
		Log::err("chroot(): %s", strerror(errnum));
		
		if (errnum == EPERM)
		{
			Log::err("Unable to change root directory, are you root?");
		}
		
		net.shutdown();
		exit(1);
	}
	if(chdir("/") == -1)
	{
		Log::err("chdir(): %s", strerror(errno));
		net.shutdown();
		exit(1);
	}
	if(setresuid(NOBODY,NOBODY,NOBODY) == -1)
	{
		Log::err("setresuid(): %s", strerror(errno));
		net.shutdown();
		exit(1);
	}
	
	switch (net.startListen())
	{		
	case Network::Err::OK:
		break;
		
	case Network::Err::Listen:
		Log::err("Failed to start listening");
		break;
		
	case Network::Err::Accept:
		Log::err("Failed to accept connection");
		break;
		
	case Network::Err::Setup:
		Log::err("Failed to setup network");
		break;
		
	case Network::Err::Process:
		Log::err("Error while processing connection");
		break;
	}
	
	Log::info("Server shutting down");
	net.shutdown();
	
	mimeFinder.shutdown();
	
	return 0;
}

