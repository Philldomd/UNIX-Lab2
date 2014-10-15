#pragma once

#include "ExecErr.h"
#include "network.h"
#include "threadpool.h"

#include <sys/epoll.h>

class ExecIOMultThreadPool
{
private:
	const static size_t MAX_EVENTS = 8;
	
	static Network* network;
	
	struct epoll_event events[MAX_EVENTS];
	int epollfd;
	
	threadpool_t* pool;

public:
	ExecErr init(Network* network);
	ExecErr shutdown();
	
	ExecErr run();
	
private:
	static void connectionTask(void* data);
};

typedef ExecIOMultThreadPool Exec;
