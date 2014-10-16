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
	static int epollfd;
	
	threadpool_t* pool;
	
	struct DataLink
	{
		Network::SocketData data;
		DataLink* next;
	};
	
	const static size_t numBuffs = 1024;
	static DataLink links[numBuffs];
	static DataLink* freeData;
	
	static bool acceptingNew;
	
	static pthread_mutex_t dataLock;

public:
	ExecErr init(Network* network);
	ExecErr shutdown();
	
	ExecErr run();
	
private:
	static void connectionTask(void* data);
	
	static void pushFreeLink(DataLink* link);
	static DataLink* popFreeLink();
};

typedef ExecIOMultThreadPool Exec;
