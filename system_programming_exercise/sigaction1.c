#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void ssu_signal_handler(int signo){
	printf("ssu_signal_handler control\n");
}

int main(void){
	struct sigaction sig_act;
	sigset_t sig_set;

	//define SIGUSR1 signal
	sigemptyset(&sig_act.sa_mask); // initialization: excluding all signals.
	sig_act.sa_flags = 0; 
	sig_act.sa_handler = ssu_signal_handler; 
	sigaction(SIGUSR1, &sig_act, NULL); 

	printf("before first kill()\n");
	kill(getpid(), SIGUSR1);

	//block SIGUSR1 signal
	sigemptyset(&sig_set); // initialization: excluding all signals.
	sigaddset(&sig_set, SIGUSR1); // add SIGUSR1 signal to sig_set.
	sigprocmask(SIG_SETMASK, &sig_set, NULL); //block SIGUSR1(which is member of sig_set)

	printf("before second kill()\n");
	kill(getpid(), SIGUSR1); //because SIGUSR1 is blocked, ssu_signal_handler will not print.
	printf("after second kill()\n");
	
	exit(0);

}
