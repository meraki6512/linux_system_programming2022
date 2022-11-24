#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

static void ssu_func(int signo);
void ssu_print_mask(const char * str);

int main(void){

	sigset_t new_mask, old_mask, wait_mask;
	//print msg and check sig members
	ssu_print_mask("program start: "); 

	//make SIGINT signal and exe ssu_func
	if (signal(SIGINT, ssu_func)==SIG_ERR){
		fprintf(stderr, "signal(SIGINT) error\n");
		exit(1);
	}

	//set wait_mask and add SIGUSR1 signal
	sigemptyset(&wait_mask);
	sigaddset(&wait_mask, SIGUSR1);
	
	//set new_mask and add SIGINT signal
	sigemptyset(&new_mask);
	sigaddset(&new_mask, SIGINT);
	//set block mask with new_mask(SIGINT)
	if (sigprocmask(SIG_BLOCK, &new_mask, &old_mask) < 0){
		fprintf(stderr, "SIG_BLOCK() error\n");
		exit(1);
	}

	//reset block mask with wait_mask(SIGUSR1) and pend ... 
	if (sigsuspend(&wait_mask)!=-1){
		fprintf(stderr, "sigsuspend() error\n");
		exit(1);
	}

	ssu_print_mask("after return from sigsuspend: ");

	//set mask with old_mask(NULL)
	if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0){
		fprintf(stderr, "SIG_SETMASK() error\n");
		exit(1);
	}

	ssu_print_mask("program exit: ");
	exit(0);
	
}

void ssu_print_mask(const char *str){

	sigset_t sig_set;
	int err_num;
	err_num = errno;

	//mask NULL
	if (sigprocmask(0, NULL, &sig_set)<0){
		printf("sigprocmask() error\n");
		exit(1);
	}

	printf("%s", str);

	//check if signals are member of sig_set

	if (sigismember(&sig_set, SIGINT))
		printf("SIGINT ");
	
	if (sigismember(&sig_set, SIGQUIT))
		printf("SIQUIT ");

	if (sigismember(&sig_set, SIGUSR1))
		printf("SIGUSR1 ");
	
	if (sigismember(&sig_set, SIGALRM))
		printf("SIGALRM ");

	printf("\n");

	errno = err_num;
}

static void ssu_func(int signo){
	ssu_print_mask("\nin ssu_func: ");
}
