#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(void){

	char buf[BUFFER_SIZE];
	int pipe_fd[2];

	//make pipe files
	//pipe_fd[0]: 3 for reading, pipe_fd[1]: 4 for writing
	if (pipe(pipe_fd)==-1){
		fprintf(stderr,"pipe error");
		exit(1);
	}

	printf("writing to file descriptor %d\n", pipe_fd[1]);
	write(pipe_fd[1], "OSLAB", 6);
	printf("reading from file descriptor %d\n", pipe_fd[0]);
	read(pipe_fd[0], buf, 6); //buf: "OSLAB"
	printf("read \"%s\" \n", buf); 

	exit(0);
}
