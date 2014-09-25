#include "config.h"
#include "daemon.h"

#include <cstdio>
#include <syslog.h>
#include <unistd.h>

int main(int argc,char* argv[])
{
	//daemonizeAndClearFiles();
	//openlog(argv[0], LOG_CONS, LOG_DAEMON);
	Config conf("conf.yaml");
	
	printf("Port: %hu\n", conf.getPortNr());
	printf("Root: %s\n", conf.getRootPath());
	//sleep(100);
	return 0;
}

