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
	
	struct DataLink
	{
		Network::SocketData data;
		DataLink* next;
	};
	
	const static size_t numBuffs = 1024;
	DataLink links[numBuffs];
	DataLink* freeData;
	
	bool acceptingNew;

public:
	ExecErr init(Network* network);
	ExecErr shutdown();
	
	ExecErr run();
	
private:
	void pushFreeLink(DataLink* link);
	DataLink* popFreeLink();
};

typedef ExecIOMult Exec;
