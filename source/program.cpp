#include "config.h"
#include "daemon.h"
#include "logger.h"
#include "MimeFinder.h"
#include "network.h"

#ifdef IO_MULT_THREAD_POOL
#	include "ExecIOMultThreadPool.h"
#elif defined(FORK)
#	include "ExecFork.h"
#else
#	include "ExecIOMult.h"
#endif

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
		if(logFile != nullptr)
		{
			Log::openFile(logFile);
		}
		else
		{
			Log::open(argv[0], LOG_CONS, LOG_DAEMON);
		}
		Log::debug("Daemon server started at port: %hu", port);
	}
	else
	{
		if(logFile != nullptr)
		{
			Log::openFile(logFile);
		}
		else
		{
			Log::open(argv[0], LOG_CONS | LOG_PERROR, LOG_USER);
		}
		Log::debug("Server started at port: %hu", port);
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
	
	if (net.startListen() != Network::Err::OK)
	{
		Log::err("Failed to start listening");
	}
	else
	{
		Exec exec;
		if (exec.init(&net) != ExecErr::OK)
		{
			Log::err("Exec init error!");
		}
		else if (exec.run() != ExecErr::OK)
		{
			Log::err("Exec run error");
		}
		else if (exec.shutdown() != ExecErr::OK)
		{
			Log::err("Exec shutdown error!");
		}
	}
	
	Log::info("Server shutting down");
	net.shutdown();
	
	mimeFinder.shutdown();
	
	return 0;
}

