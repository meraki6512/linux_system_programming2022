#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static void ssu_signal_handler1(int signo);
static void ssu_signal_handler2(int signo);

int main(void){
	
	struct sigaction act_int, act_quit;

	//define SIGINT signal
	act_int.sa_flags = 0;
	act_int.sa_handler = ssu_signal_handler1;
	sigemptyset(&act_int.sa_mask); // initialization: excluding all signals.
	sigaddset(&act_int.sa_mask, SIGQUIT); //add SIGQUIT to set(act_int.sa_mask)

	if (sigaction(SIGINT, &act_int, NULL)<0){
		fprintf(stderr, "sigaction(SIGINT) error\n");
		exit(1);
	}
	
	//define SIGQUIT signal
	act_quit.sa_flags = 0;
	act_quit.sa_handler = ssu_signal_handler2;
	sigemptyset(&act_quit.sa_mask); // initialization: excluding all signals.
	sigaddset(&act_quit.sa_mask, SIGINT); //add SIGINT to set(act_int.sa_mask)

	if (sigaction(SIGQUIT, &act_quit, NULL)<0){
		fprintf(stderr, "sigaction(SIGQUIT) error\n");
		exit(1);
	}
	
	pause();
	exit(0);
}

static void ssu_signal_handler1(int signo){
	printf("Signal handler of SIGINT: %d\n", signo);
	printf("SIGQUIT signal is blocked: %d\n", signo);
	printf("sleeping 3 sec\n");
	sleep(3);
	printf("Signal handler of SIGINT ended\n");
}

static void ssu_signal_handler2(int signo){
	printf("Signal handler of SIGQUIT: %d\n", signo);
	printf("SIGINT signal is blocked: %d\n", signo);
	printf("sleeping 3 sec\n");
	sleep(3);
	printf("Signal handler of SIGQUIT ended\n");
}
