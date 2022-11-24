#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void ssu_check_pending(int signo, char* signame);
void ssu_signal_handler(int signo);

int main(void){
	struct sigaction sig_act;
	sigset_t sig_set;

	//define SIGUSR1 signal
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = 0;
	sig_act.sa_handler = ssu_signal_handler;

	if (sigaction(SIGUSR1, &sig_act, NULL)!=0){
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}
	else{
		//block SIGUSR1 signal
		sigemptyset(&sig_set); // initialization: excluding all signals.
		sigaddset(&sig_set, SIGUSR1); // add SIGUSR1 signal to sig_set.
		if (sigprocmask(SIG_SETMASK, &sig_set, NULL)){ //block SIGUSR1(which is member of sig_set)
			fprintf(stderr, "sigprocmask() error\n");
			exit(1);
		}
		else{
			printf("SIGUSR1 signals are now blocked\n");
			kill(getpid(), SIGUSR1); //because SIGUSR1 is blocked, ssu_signal_handler will not print.
			printf("after kill()\n");
			ssu_check_pending(SIGUSR1, "SIGUSR1");
			
			//unblock SIGUSR1 signal
			sigemptyset(&sig_set);
			sigprocmask(SIG_SETMASK, &sig_set, NULL);
			//because process is killed before unblocked, ssu_signal_handler will print here.
			printf("SIGUSR1 signals are no longer blocked\n"); //unblock SIGUSR1(which is member of sig_set)
			ssu_check_pending(SIGUSR1, "SIGUSR1"); 
		}
	}

	exit(0);
}

void ssu_check_pending(int signo, char* signame){
	sigset_t sig_set;

	if (sigpending(&sig_set)!=0) //if sigpending successed ... (: if any signal that is blocked && pending is generated ...)
		printf("sigpending() error\n");
	else if (sigismember(&sig_set, signo)) //if signo is member of pendingset ... (: if signo is blocked && pending ...)
		printf("a %s signal is pending\n", signame);
	else
		printf("%s signals are not pending\n", signame);
}

void ssu_signal_handler(int signo){
	printf("in ssu_signal_handler function\n");
}
