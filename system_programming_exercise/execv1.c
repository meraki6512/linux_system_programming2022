#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/resource.h>

double ssu_maketime(struct timeval *time);
void term_stat(int stat);
void ssu_print_child_info(int stat, struct rusage * rusage);

int main(void){

	struct rusage rusage;
	pid_t pid;
	int status;

	//fork
	if ((pid=fork())==0){ //child process
		char* args[] = {"find", "/", "-maxdepth", "4", "-name", "stdio.h", NULL};
		//exec with arguments vector
		if (execv("/usr/bin/find", args)<0){
			fprintf(stderr, "execv error\n");
			exit(1);
		}
	}

	//wait until child process terminated
	if (wait3(&status, 0, &rusage)==pid){ //wait3: similar to wait function (wait for termination of random child process)
		ssu_print_child_info(status, &rusage); 
	}
	else{ //if there is no child process to wait for
		fprintf(stderr, "wait3 error\n");
		exit(1);
	}
	
	exit(0);
}


//get tv_sec + tv_usec/1000000 value from time structure
double ssu_maketime(struct timeval* time){ 
	return ((double)time->tv_sec + time->tv_usec/1000000.0);
}

//print termination status with macro (WEXITSTATUS, WTERMSIG, WCOREDUMP, WSTOPSIG)
void term_stat(int stat){
	if (WIFEXITED(stat))
		printf("normally terminated. exit status = %d\n", WEXITSTATUS(stat));
	else if (WIFSIGNALED(stat))
		printf("abnormal termination by signal %d. %s\n", WTERMSIG(stat), 
#ifdef WCOREDUMP
				WCOREDUMP(stat)?"core dumped":"no core"
#else
				NULL
#endif
			  );
	else if (WIFSTOPPED(stat))
		printf("stopped by signal %d\n", WSTOPSIG(stat));
}

//print cpu utime child process used (in user mode and system mode)
void ssu_print_child_info(int stat, struct rusage *rusage){
	printf("Termination info follows\n");
	term_stat(stat);
	printf("user CPU time: %.2f(sec)\n", ssu_maketime(&rusage->ru_utime));
	printf("system CPU time: %.2f(sec)\n", ssu_maketime(&rusage->ru_stime));
}

