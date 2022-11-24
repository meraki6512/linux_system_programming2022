#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

//#define FIFO_NAME "american_maid"
#define FIFO_NAME "ssu_fifofile"

int main(void){
	
	char s[300];
	int num, fd;

	//make FIFO file (FIFO: named pipe, name: FIFO_NAME)
	mkfifo(FIFO_NAME, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IWOTH|S_IROTH);

	printf("waiting for readers...\n");
	//fd = open(FIFO_NAME, O_RDWR);
	fd = open(FIFO_NAME, O_WRONLY);
	printf("got a reader--type some stuff\n");

	while(fgets(s, 1024, stdin), !feof(stdin)){ //get string from stdin
		if ((num = write(fd, s, strlen(s)-1))==-1) //write string to fd(fifo file)
			perror("write");
		else
			printf("speak: wrote %d bytes\n", num);
	}

	exit(0);

}
