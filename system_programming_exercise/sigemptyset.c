#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(void){

	sigset_t set;
	
	sigemptyset(&set); // (set) initialization: excluding all signals.
	sigaddset(&set, SIGINT); // add SIGINT signal to set.

	switch(sigismember(&set, SIGINT)){ //if SIGINT is member of set...
		case 1: //TRUE
			printf("SIGINT is included.\n");
			break;
		case 0: //FALSE
			printf("SIGINT is not included.\n");
			break;
		default: //ERR
			printf("failed to call sigismember()\n");
	}

	switch(sigismember(&set,SIGSYS)){ //if SIGSYS is member of set
		case 1: //TRUE
			printf("SIGSYS is included.\n");
			break;
		case 0: //FALSE
			printf("SIGSYS is not included.\n");
			break;
		default: //ERR
			printf("failed to call sigismember()\n");
	}

	exit(0);
}
