#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <errno.h>

static void ssu_alarm(int signo);
static void ssu_func(int signo);
void ssu_mask(const char* str);

static volatile sig_atomic_t can_jump;
static sigjmp_buf jump_buf;

int main(void){

	//make SIGUSR1 signal and exe ssu_func
	if (signal(SIGUSR1, ssu_func) == SIG_ERR){
		fprintf(stderr, "SIGUSR1 error\n");
		exit(1);
	}

	//make SIGALRM signal and exe ssu_alarm
	if (signal(SIGALRM, ssu_alarm) == SIG_ERR){
		fprintf(stderr, "SIGALRM error\n");
		exit(1);
	}

	//print msg and check sig members
	ssu_mask("starting main: "); 

	//if siglongjmp occurs, terminate
	if (sigsetjmp(jump_buf, 1)){
		ssu_mask("ending main: ");
		exit(0);
	}

	can_jump = 1;

	while(1)
		pause();

	exit(0);
}

void ssu_mask(const char* str){
	
	sigset_t sig_set;
	int err_num;
	err_num = errno;

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
	time_t start_time;
	
	if (can_jump == 0) //jump only on the first time
		return;

	ssu_mask("starting ssu_func: ");
	alarm(3); //do alarm()
	start_time = time(NULL); //save current time

	while (1)
		if (time(NULL) > start_time + 5) //if 5 sec have passed (from saved time), break
			break;

	ssu_mask("ending ssu_func: ");

	can_jump = 0;

	siglongjmp(jump_buf, 1); //jump to jumpbuf and return 1
}

static void ssu_alarm(int signo){
	ssu_mask("in ssu_alarm: ");
}










