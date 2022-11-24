#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(void){
	
	char buf[BUFFER_SIZE];
	int pid;
	int pipe_fd[2];

	//make pipe files (read, write)
	if (pipe(pipe_fd)<0){
		fprintf(stderr, "pipe error\n");
		exit(1);
	}

	//fork
	if ((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid == 0){ //if child:
		close(pipe_fd[0]); //close fd for reading

		while (1){ //write
			memset(buf, 0x00, BUFFER_SIZE);
			sprintf(buf, "Hello Mother Process. My name is %d\n", getpid());
			write(pipe_fd[1], buf, strlen(buf));
			sleep(1);
		}
	}
	else{ //if parent:
		close(pipe_fd[1]); //close fd for writing

		while(1){ //read
			memset(buf, 0x00, BUFFER_SIZE);
			read(pipe_fd[0], buf, BUFFER_SIZE);
			fprintf(stderr, "%s", buf);
		}
	}

	exit(0);
}
