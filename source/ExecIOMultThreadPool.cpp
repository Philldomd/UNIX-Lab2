#include "ExecIOMultThreadPool.h"

#include "logger.h"

#include <cerrno>
#include <cstring>

#define NUM_THRD 7
#define QUEUE_ELEM 1024

Network* ExecIOMultThreadPool::network = nullptr;

ExecErr ExecIOMultThreadPool::init(Network* network)
{
	this->network = network;
	
	if((epollfd = epoll_create1(0)) ==  -1 )
	{
		Log::err("epoll_create1(): %s", strerror(errno));
		return ExecErr::GenericFailure;
	}
	
	struct epoll_event ev = {};
	ev.events = EPOLLIN;
	ev.data.fd = network->getAcceptSocket();
	
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
	{
		Log::err("epoll_ctl(): %s", strerror(errno));
		return ExecErr::GenericFailure;
	}
	
	pool = threadpool_create(NUM_THRD, QUEUE_ELEM);
	if (pool == nullptr)
	{
		Log::err("threadpool_create(): Unknown error");
		return ExecErr::GenericFailure;
	}
	
	return ExecErr::OK;
}

ExecErr ExecIOMultThreadPool::shutdown()
{
	int err = threadpool_destroy(pool, threadpool_graceful);
	if (err != 0)
	{
		Log::err("threadpool_destroy(): %d", err);
	}
	
	return ExecErr::OK;
}

ExecErr ExecIOMultThreadPool::run()
{
	while (true)
	{
		int nfds;
		if((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1)
		{
			Log::err("epoll_wait(): %s", strerror(errno));
			return ExecErr::GenericFailure;
		}
		
		for(int n = 0; n < nfds; ++n)
		{
			int acceptedSocket;
			if(events[n].data.fd == network->getAcceptSocket())
			{
				if((acceptedSocket = network->accept()) == -1)
				{
					Log::err("accept(): %s", strerror(errno));
					return ExecErr::Network;
				}
				
				struct epoll_event ev = {};
				ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
				ev.data.fd = acceptedSocket;
				if(epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptedSocket, &ev) == -1)
				{
					Log::err("epoll_ctl(): %s", strerror(errno));
					return ExecErr::GenericFailure;
				}
			}
			else
			{
				threadpool_add(pool, ExecIOMultThreadPool::connectionTask, (void*)events[n].data.fd, threadpool_blocking);
			}
		}
	}
}

void ExecIOMultThreadPool::connectionTask(void* data)
{
	network->handleConnection((int)data);
}
