#include "daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

void daemonize()
{
	if (getppid() == 1)
	{
		return;
	}

#ifdef SIGTTOU
	// ignore SIGTTOU
#endif

#ifdef SIGTTIN
	// ignore SIGTTIN
#endif
	
#ifdef SIGTSTP
	// ignore SIGTSTP
#endif

	if (fork() != 0)
	{
		exit(0);
	}
		
	setsid();

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGHUP, &sa, NULL);
	
	if(fork() != 0)
	{
		exit(0);
	}
}

void daemonizeAndClearFiles()
{
	rlimit rl;
	if(getrlimit(RLIMIT_NOFILE, &rl))
	{
		perror("getrlimit()");
	}
		
	daemonize();
	
	if(rl.rlim_max == RLIM_INFINITY)
	{
		rl.rlim_max = 1024;
	}
	
	for (unsigned int fd = 0; fd < rl.rlim_max; fd++)
	{
		close(fd);
	}
	
	chdir("/");
	
	umask(0);
	
	open("/dev/null", O_RDWR);
	dup(0);
	dup(0);
}
