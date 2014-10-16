#include "ExecIOMultThreadPool.h"

#include "logger.h"

#include <cerrno>
#include <cstring>

#define NUM_THRD 7
#define QUEUE_ELEM (2 * NUM_THRD)

Network* ExecIOMultThreadPool::network = nullptr;
ExecIOMultThreadPool::DataLink ExecIOMultThreadPool::links[numBuffs];
int ExecIOMultThreadPool::epollfd;
bool ExecIOMultThreadPool::acceptingNew;
ExecIOMultThreadPool::DataLink* ExecIOMultThreadPool::freeData;
pthread_mutex_t ExecIOMultThreadPool::dataLock;

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
	
	acceptingNew = true;
	
	freeData = nullptr;
	
	for (DataLink& link : links)
	{
		pushFreeLink(&link);
	}
	
	pthread_mutex_init(&dataLock, NULL);
	
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
				pthread_mutex_lock(&dataLock);
				DataLink* link = popFreeLink();
				if (link == nullptr)
				{
					if (epoll_ctl(epollfd, EPOLL_CTL_DEL, network->getAcceptSocket(), NULL) == -1)
					{
						Log::err("epoll_ctl(): %s", strerror(errno));
						pthread_mutex_unlock(&dataLock);
						return ExecErr::GenericFailure;
					}
					acceptingNew = false;
					pthread_mutex_unlock(&dataLock);
					continue;
				}
				pthread_mutex_unlock(&dataLock);
				
				if((acceptedSocket = network->accept()) == -1)
				{
					Log::err("accept(): %s", strerror(errno));
					return ExecErr::Network;
				}
				
				link->data.fd = acceptedSocket;
				link->data.readLen = 0;
				
				struct epoll_event ev = {};
				ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
				ev.data.ptr = (void*)link;
				if(epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptedSocket, &ev) == -1)
				{
					Log::err("epoll_ctl(): %s", strerror(errno));
					return ExecErr::GenericFailure;
				}
			}
			else
			{
				threadpool_add(pool, ExecIOMultThreadPool::connectionTask, (void*)events[n].data.ptr, threadpool_blocking);
			}
		}
	}
}

void ExecIOMultThreadPool::connectionTask(void* data)
{
	DataLink* link = (DataLink*)data;
	
	switch (network->handleConnection(&link->data))
	{
	case Network::Err::ReadMore:
		{
			struct epoll_event ev = {};
			ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
			ev.data.ptr = (void*)link;
			if (epoll_ctl(epollfd, EPOLL_CTL_MOD, link->data.fd, &ev) == -1)
			{
				Log::err("epoll_ctl(): %s", strerror(errno));
				return;
			}
		}
		break;
		
	case Network::Err::OK:
	case Network::Err::Process:
	default:
		pthread_mutex_lock(&dataLock);
		pushFreeLink(link);
		if (acceptingNew == false)
		{
			struct epoll_event ev = {};
			ev.events = EPOLLIN;
			ev.data.fd = network->getAcceptSocket();
			
			if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
			{
				Log::err("epoll_ctl(): %s", strerror(errno));
				pthread_mutex_unlock(&dataLock);
				return;
			}
			
			acceptingNew = true;
		}
		pthread_mutex_unlock(&dataLock);
		break;
	}
}

void ExecIOMultThreadPool::pushFreeLink(DataLink* link)
{
	link->next = freeData;
	freeData = link;
}

ExecIOMultThreadPool::DataLink* ExecIOMultThreadPool::popFreeLink()
{
	DataLink* link = freeData;
	if (link != nullptr)
	{
		freeData = link->next;
	}
	return link;
}
