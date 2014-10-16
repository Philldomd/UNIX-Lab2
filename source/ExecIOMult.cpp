#include "ExecIOMult.h"

#include "logger.h"

#include <cerrno>
#include <cstring>

ExecErr ExecIOMult::init(Network* network)
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
	
	acceptingNew = true;
	
	freeData = nullptr;
	
	for (DataLink& link : links)
	{
		pushFreeLink(&link);
	}
	
	return ExecErr::OK;
}

ExecErr ExecIOMult::shutdown()
{
	return ExecErr::OK;
}

ExecErr ExecIOMult::run()
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
				DataLink* link = popFreeLink();
				if (link == nullptr)
				{
					if (epoll_ctl(epollfd, EPOLL_CTL_DEL, network->getAcceptSocket(), NULL) == -1)
					{
						Log::err("epoll_ctl(): %s", strerror(errno));
						return ExecErr::GenericFailure;
					}
					acceptingNew = false;
					continue;
				}
				
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
				DataLink* link = (DataLink*)events[n].data.ptr;
				
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
							return ExecErr::GenericFailure;
						}
					}
					break;
					
				case Network::Err::OK:
				case Network::Err::Process:
				default:
					pushFreeLink(link);
					if (acceptingNew == false)
					{
						struct epoll_event ev = {};
						ev.events = EPOLLIN;
						ev.data.fd = network->getAcceptSocket();
						
						if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
						{
							Log::err("epoll_ctl(): %s", strerror(errno));
							return ExecErr::GenericFailure;
						}
						
						acceptingNew = true;
					}
					break;
				}
			}
		}
	}
}

void ExecIOMult::pushFreeLink(DataLink* link)
{
	link->next = freeData;
	freeData = link;
}

ExecIOMult::DataLink* ExecIOMult::popFreeLink()
{
	DataLink* link = freeData;
	if (link != nullptr)
	{
		freeData = link->next;
	}
	return link;
}
