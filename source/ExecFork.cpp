#include "ExecFork.h"

#include "logger.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void sigchldHandler(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
	}
}
    
ExecErr ExecFork::init(Network* network)
{
	this->network = network;
	
	struct sigaction act = {};
	act.sa_handler = sigchldHandler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);
	
	return ExecErr::OK;
}

ExecErr ExecFork::shutdown()
{
	return ExecErr::OK;
}

ExecErr ExecFork::run()
{
	while (true)
	{
		int acceptedSocket = network->accept();
		if (acceptedSocket == -1)
		{
			Log::err("accept(): %s", strerror(errno));
			return ExecErr::Network;
		}
		
		if (fork() == 0)
		{
			Network::SocketData data;
			data.fd = acceptedSocket;
			data.readLen = 0;
			while (true)
			{
				switch(network->handleConnection(&data))
				{
				case Network::Err::ReadMore:
					break;
					
				case Network::Err::OK:
				case Network::Err::Process:
				default:
					exit(0);
				}
			}
		}
		else
		{
			if (close(acceptedSocket) == -1)
			{
				Log::err("close(): %s", strerror(errno));
			}
		}
	}
	
	return ExecErr::OK;
}
