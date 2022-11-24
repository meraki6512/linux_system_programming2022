#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

int ssu_daemon_init(void);

int main(void){
	
	pid_t pid;
	pid = getpid();
	printf("parent process: %d\n", pid);
	printf("daemon process initializaion\n");

	if (ssu_daemon_init()<0){
		fprintf(stderr, "ssu_daemon_init failed\n");
		exit(1);
	}

	while(1){
		//check errors through the log
		openlog("lpd", LOG_PID, LOG_LPR);
		syslog(LOG_ERR, "open failed lpd %d\n", 1);
		closelog();
		sleep(5);
	}

	exit(0);
}

int ssu_daemon_init(void){
	
	pid_t pid;
	int fd, maxfd;

	// 1. on background: terminate after fork()
	if ((pid=fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid!=0)
		exit(0);

	pid = getpid();
	printf("process %d running as daemon\n", pid);

	// 2. leave the group (make new)
	setsid();
	
	// 3. ignore IO signals
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	maxfd = getdtablesize();
	for (fd = 0; fd<maxfd; fd++)
		close;

	// 4. unmask 
	umask(0);
	
	// 5. change current directory as root
	chdir("/");

	// 6 & 7. close all fd & redirect std(in, out, err) to /dev/null
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	return 0;


}


