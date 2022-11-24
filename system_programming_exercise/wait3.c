#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>

int main(void){

	//fork
	if (fork()==0) //child process
		execl("/bin/echo", "echo", "this is", "message one", (char*)0); //exec (with list of arguments)

	//fork again
	if (fork()==0) //another child process
		execl("/bin/echo", "echo", "this is", "message two", (char*)0); //exec (with list of arguments)

	printf("parent: waiting for chlidren\n"); 
	
	//until wait catch error (: no more child process to wait for)
	while (wait((int*)0)==-1);

	printf("parent: all children terminated\n");
	exit(0);

}
