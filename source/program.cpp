#include "daemon.h"

#include <syslog.h>
#include <unistd.h>

int main(int argc,char* argv[])
{
	daemonizeAndClearFiles();
	openlog(argv[0], LOG_CONS, LOG_DAEMON);
	sleep(100);
	return 0;
}

