#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>

#define EXIT_CODE 1


int main(void){
	
	pid_t pid;
	int ret_val, status;

	//fork
	if ((pid = fork())==0){ //child process
		printf("child pid=%d, ppid=%d, exit_code=%d\n", getpid(), getppid(), EXIT_CODE);
		exit(EXIT_CODE);  
	}

	printf("parent: waiting for child=%d\n", pid);
	//wait until child process exits
	//(ret_val = child process id)
	//(higher 8 bits of status = factor of child process's exit(EXIT_CODE))
	ret_val = wait(&status); 
	printf("parent: return value=%d\n", ret_val);
	printf("child's status=%x\n", status);
	printf("and shifted=%x\n", (status>>8)); //shift lower 8 bits of status

	exit(0);
}
