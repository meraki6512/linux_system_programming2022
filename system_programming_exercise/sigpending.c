#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(void){

	sigset_t pendingset; 
	sigset_t sig_set;
	int count = 0;

	sigemptyset(&sig_set); // (set) initialization: excluding all signals.
	sigaddset(&sig_set, SIGINT); // add SIGINT signal to set.
	sigprocmask(SIG_BLOCK, &sig_set, NULL); //block SIGINT(which is member of sig_set)

	while(1){
		printf("count: %d\n", count++);
		sleep(1);

		if (sigpending(&pendingset)==0){ //if sigpending successed ... (: if any signal that is blocked && pending is generated ...)
			if (sigismember(&pendingset, SIGINT)){ //if SIGINT is member of pendingset ... (: if SIGINT is blocked && pending ...)
				printf("waiting... SIGINT is blocked. end loop.\n");
				break;
			}
		}
	}

	exit(0);
}
