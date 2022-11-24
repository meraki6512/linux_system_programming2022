#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>

int main(void){
	
	pid_t child1, child2;
	int pid, status;

	//fork
	if ((child1=fork())==0) //child process
		execlp("date", "date", (char*)0); //exec (with arguments list including filename)

	//fork again
	if ((child2=fork())==0) //another child process
		execlp("who", "who", (char*)0); //exec (with arguments list including filename)

	printf("parent: waiting for children\n");

	//until wait catch error (: no more child process to wait for)
	//(pid: child process id or -1(: error))
	while ((pid = wait(&status))!=-1){
		if (pid == child1) //if child1 process exit
			printf("parent: first child: %x\n", (status>>8));

		else if (pid == child2) //if child2 process exit
			printf("parent: second child: %x\n", (status>>8));
	}

	printf("parent: all children process terminated\n");
	exit(0);
}
