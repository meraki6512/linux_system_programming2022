#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(void){

	sigset_t sig_set;
	int count;

	sigemptyset(&sig_set); // (set) initialization: excluding all signals.
	sigaddset(&sig_set, SIGINT); // add SIGINT signal to set.

	sigprocmask(SIG_BLOCK, &sig_set, NULL); //block SIGINT(which is member of sig_set)

	for (count=3; 0<count; count--){ 
		printf("count %d\n", count);
		sleep(1);
	}

	printf("unblock Ctrl-C\n");
	sigprocmask(SIG_UNBLOCK, &sig_set, NULL); //unblock SIGINT(which is member of sig_set)
	printf("not printed if you pressed ctrl-c while counting\n");

	while(1);
	exit(0);
}
