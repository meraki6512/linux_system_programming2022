#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void ssu_echo_exit(int status);

int main(void){
	
	pid_t pid;
	int status;

	//ex1)
	//fork
	if ((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid==0) //child process
		exit(7); //child process: normally terminated

	if (wait(&status)<0){ //status: lower 8 bits of exit factor(7)
		fprintf(stderr, "wait error\n");
		exit(1);
	}

	ssu_echo_exit(status); //parent process: print status info with WEXITSTATUS macro

	//ex2)
	//fork again
	if ((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid==0) //child process
		abort(); //child process: abnormal termination

	if (wait(&status)<0){ //status & 7f: signal number
		fprintf(stderr, "wait error\n");
		exit(1);
	}

	ssu_echo_exit(status);  //parent process: print status info with WTERMSIG macro

	//ex3)
	//fork again
	if ((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid==0) //child process
		status/=0; //child process: abnormal termination

	if (wait(&status)<0){ //status & 7f: signal number
		fprintf(stderr, "wait error\n");
		exit(1);
	}

	ssu_echo_exit(status);  //parent process: print status info with WTERMSIG macro
	exit(0);
}

void ssu_echo_exit(int status){
	if (WIFEXITED(status))
		printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("abnormal termination, signal number = %d%s\n", WTERMSIG(status), 
#ifdef WCOREDUMP

				WCOREDUMP(status) ? "(core file generated)" : ""
#else
					""
#endif
			  );
	else if (WIFSTOPPED(status))
		printf("child stopped, signal number = %d\n", WSTOPSIG(status)); 

}
