#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(void){
	
	sigset_t old_set;
	sigset_t sig_set;

	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGINT); //add SIGINT signal to sig_set
	sigprocmask(SIG_BLOCK, &sig_set, &old_set); //block sig_set(SIGINT)
	sigsuspend(&old_set); //reset block signals with old_set (unblock SIGINT) and pause...

	exit(0);

}
