#pragma once

#include "ExecErr.h"
#include "network.h"

#include <sys/epoll.h>

class ExecIOMult
{
private:
	const static size_t MAX_EVENTS = 8;
	
	Network* network;
	
	struct epoll_event events[MAX_EVENTS];
	int epollfd;

public:
	ExecErr init(Network* network);
	ExecErr shutdown();
	
	ExecErr run();
};

typedef ExecIOMult Exec;
