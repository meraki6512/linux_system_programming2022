#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(void){

	char buf[BUFFER_SIZE];
	int pid;
	int pipe_fd[2];

	//make pipe files (read, write)
	pipe(pipe_fd);

	if ((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid>0){ //fork: parent
		printf(" PARENT: writing to the pipe\n");
		write(pipe_fd[1], "OSLAB", 6); 
		printf(" PARENT: waiting\n");
		wait(NULL); //wait until child process terminated
	}
	else{ //fork: child
		printf(" CHILD: reading from pipe\n");
		read(pipe_fd[0], buf, 6); //buf: "OSLAB"
		printf(" CHILD: read \"%s\" \n", buf);
		exit(0);
	}

	exit(0);
}
